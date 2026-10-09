#!/usr/bin/env python3
"""Plan and generate native SVXReflector federation configuration.

Planning files contain no authentication keys.  The builder never installs a
file into /etc and never restarts SVXReflector.
"""

from __future__ import annotations

import argparse
import copy
import datetime as dt
import difflib
import getpass
import json
import os
import re
import sys
from pathlib import Path
from typing import Any


PLAN_SCHEMA = 1
OWNERSHIP = ("domain", "peer", "network-wide")
DISTRIBUTION = ("local", "selected-peers", "network-wide")


def ask(prompt: str, default: str | None = None,
        required: bool = True) -> str:
    suffix = f" [{default}]" if default is not None else ""
    while True:
        value = input(f"{prompt}{suffix}: ").strip()
        if not value and default is not None:
            return default
        if value or not required:
            return value
        print("A value is required.")


def ask_bool(prompt: str, default: bool = False) -> bool:
    marker = "Y/n" if default else "y/N"
    while True:
        value = input(f"{prompt} [{marker}]: ").strip().lower()
        if not value:
            return default
        if value in ("y", "yes", "1", "true"):
            return True
        if value in ("n", "no", "0", "false"):
            return False
        print("Please answer yes or no.")


def ask_int(prompt: str, default: int | None = None,
            minimum: int = 0, maximum: int | None = None) -> int:
    while True:
        raw = ask(prompt, str(default) if default is not None else None)
        try:
            value = int(raw)
        except ValueError:
            print("Enter a whole number.")
            continue
        if value < minimum:
            print(f"Enter a value of at least {minimum}.")
            continue
        if maximum is not None and value > maximum:
            print(f"Enter a value no greater than {maximum}.")
            continue
        return value


def choose(prompt: str, labels: list[str], default: int | None = None) -> int:
    if not labels:
        raise ValueError("No choices are available")
    print(f"\n{prompt}")
    for number, label in enumerate(labels, 1):
        print(f"  {number}. {label}")
    while True:
        raw = ask("Selection", str(default) if default is not None else None)
        try:
            selected = int(raw)
        except ValueError:
            print("Enter one of the displayed numbers.")
            continue
        if 1 <= selected <= len(labels):
            return selected - 1
        print("Enter one of the displayed numbers.")


def choose_many(prompt: str, peers: list[dict[str, Any]],
                defaults: list[str] | None = None,
                allow_none: bool = False) -> list[str]:
    names = [str(peer["name"]) for peer in peers]
    selected_defaults = set(defaults or [])
    print(f"\n{prompt}")
    for number, name in enumerate(names, 1):
        marker = "selected" if name in selected_defaults else "not selected"
        print(f"  {number}. {name} [{marker}]")
    while True:
        default_numbers = ",".join(
            str(index + 1) for index, name in enumerate(names)
            if name in selected_defaults)
        raw = ask(
            "Peer numbers separated by commas"
            + (" (blank selects none)" if allow_none else ""),
            default_numbers, required=not allow_none)
        if not raw:
            return []
        try:
            numbers = [int(item.strip()) for item in raw.split(",")]
        except ValueError:
            print("Enter only displayed peer numbers separated by commas.")
            continue
        if any(number < 1 or number > len(names) for number in numbers):
            print("One or more selections are outside the displayed list.")
            continue
        result: list[str] = []
        for number in numbers:
            name = names[number - 1]
            if name not in result:
                result.append(name)
        if result or allow_none:
            return result


def safe_name(value: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9_.-]+", "-", value).strip("-.")
    return cleaned or "reflector"


def json_text(value: Any) -> str:
    return json.dumps(value, indent=2) + "\n"


def write_text(path: Path, value: str, mode: int,
               replace: bool = False) -> None:
    if path.exists() and not replace:
        raise FileExistsError(f"File already exists: {path}")
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(value, encoding="utf-8")
    os.chmod(temporary, mode)
    temporary.replace(path)


def write_json(path: Path, value: Any, mode: int = 0o644,
               replace: bool = False) -> None:
    write_text(path, json_text(value), mode, replace)


def load_plan(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise ValueError(f"Plan not found: {path}") from exc
    except json.JSONDecodeError as exc:
        raise ValueError(f"Invalid JSON in {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ValueError("Plan root must be a JSON object")
    return value


def print_identity_intro() -> None:
    print("\nStep 1 of 3: Local reflector identity")
    print("-" * 45)
    print("The planning JSON contains no authentication keys.")
    print("A DNS federation service hostname is used as REFLECTOR_ID. It is not")
    print("a website URL and must not include https:// or a page path.\n")


def ask_reflector() -> dict[str, Any]:
    print_identity_intro()
    print("Federation domain")
    print("Enter the administrative network or geographical domain represented")
    print("by this reflector. Examples: GREAT-BRITAIN, NORTH-AMERICA, AUSTRALIA.")
    domain = ask("Federation domain").upper()

    print("\nLocal federation service hostname")
    print("Enter the permanent DNS hostname of this SVXReflector federation")
    print("service. Do not enter its public website URL.")
    hostname = ask("Federation service hostname")

    print("\nOutgoing federation login identity")
    print("Enter the dedicated service identity used by this reflector when it")
    print("authenticates to peers. It is not an amateur user's callsign.")
    print("Example: REFLECTOR-FGB")
    callsign = ask("Outgoing federation identity").upper()

    print("\nFederation policy file location")
    print("This is the final installation path written as LIBRARY in")
    print("Federation.conf. The standard path should normally be accepted.")
    library = ask("Federation policy file", "/etc/svxlink/federation.json")

    print("\nInitial federation state")
    print("Choose No while preparing or reviewing a deployment. This setting")
    print("takes effect only after installation and an SVXReflector restart.")
    enable = ask_bool("Enable federation in the generated configuration", False)
    return {
        "domain": domain,
        "reflector_id": hostname,
        "callsign": callsign,
        "library": library,
        "enable": enable,
    }


def ask_peer(existing_names: set[str]) -> dict[str, Any] | None:
    print("\nPeer relationship approval")
    print("A peer is another federation-enabled SVXReflector authorised to")
    print("communicate directly with this reflector. Imported audio is not relayed.")
    if not ask_bool("Has the remote manager approved this direct relationship", False):
        print("Peer not added.")
        return None

    while True:
        name = ask("Peer policy name (for example NORTH-AMERICA)").upper()
        if name not in existing_names:
            break
        print(f"Peer already exists: {name}")

    print("\nRemote federation service hostname")
    print("Enter the DNS hostname used to reach the remote SVXReflector service.")
    print("It will also be used as the expected remote REFLECTOR_ID.")
    host = ask("Remote federation service hostname")
    port = ask_int("Remote SVXReflector TCP/UDP service port", 35300, 1, 65535)
    print("Federation connection authentication: protocol 2, callsign and password")

    print(f"\nOutgoing connection to {name}")
    print("For bidirectional federation, each reflector normally maintains its")
    print("own outgoing connection to the other.")
    connect = ask_bool(f"Maintain an outgoing connection to {name}", True)

    print(f"\nIncoming login identity for {name}")
    print(f"Enter the service identity that {name} uses when connecting to this")
    print("reflector. It must be registered and authenticated locally.")
    trust = ask(f"Incoming identity used by {name}").upper()
    return {
        "name": name,
        "host": host,
        "reflector_id": host,
        "port": port,
        "protocol": 2,
        "connect": connect,
        "trust_callsign": trust,
    }


def print_peer_summary(peers: list[dict[str, Any]]) -> None:
    print("\nConfigured direct peers:")
    if not peers:
        print("  None")
        return
    for number, peer in enumerate(peers, 1):
        mode = "outgoing connector" if peer.get("connect") else "incoming only"
        print(f"  {number}. {peer['name']:<20} {peer['host']} ({mode})")


def collect_peers() -> list[dict[str, Any]]:
    print("\nStep 2 of 3: Direct federation peers")
    print("-" * 45)
    print("Configure every approved reflector with which this reflector may")
    print("exchange talkgroup traffic directly.\n")
    peers: list[dict[str, Any]] = []
    while True:
        peer = ask_peer({str(item["name"]) for item in peers})
        if peer is not None:
            peers.append(peer)
        print_peer_summary(peers)
        if peers and ask_bool("Are all required direct peers listed", False):
            return peers
        if not ask_bool("Add a remote reflector peer", not peers):
            if peers:
                return peers
            print("At least one peer is required for a federation plan.")


def ownership_for_tg(domain: str, peers: list[dict[str, Any]]) -> tuple[str, str]:
    labels = [
        f"This domain - managed by {domain}",
        "A remote peer - managed by a configured remote domain",
        "Network-wide - shared service belonging to the reflector family",
    ]
    selected = choose("Administrative ownership", labels, 1)
    if selected == 0:
        return "domain", domain
    if selected == 2:
        return "network-wide", "NETWORK-WIDE"
    peer_index = choose(
        "Select the peer with administrative responsibility",
        [str(peer["name"]) for peer in peers])
    return "peer", str(peers[peer_index]["name"])


def distribution_for_tg(peers: list[dict[str, Any]], tg: int) -> tuple[str, list[str]]:
    labels = [
        "Local only - no federation route",
        "Selected peers - share with specifically approved peers",
        "All approved peers - share with every peer in this plan",
    ]
    selected = choose("Talkgroup distribution", labels, 1)
    if selected == 0:
        return "local", []
    if selected == 2:
        defaults = [str(peer["name"]) for peer in peers]
        print("\nAll configured peers are proposed. Each relationship remains")
        print("subject to bilateral manager approval.")
        shared = choose_many(
            f"Peers sharing TG {tg} in both directions", peers, defaults)
        return "network-wide", shared
    shared = choose_many(
        f"Peers sharing TG {tg} in both directions", peers)
    return "selected-peers", shared


def ask_talkgroup(domain: str, peers: list[dict[str, Any]],
                  existing_tgs: set[int]) -> dict[str, Any] | None:
    raw = ask("Talkgroup number, or press Enter to finish", required=False)
    if not raw:
        return None
    try:
        tg = int(raw)
    except ValueError:
        print("Talkgroup must be a positive whole number.")
        return {}
    if tg <= 0:
        print("Talkgroup must be greater than zero.")
        return {}
    if tg in existing_tgs:
        print(f"Talkgroup already exists: {tg}")
        return {}
    description = ask("Talkgroup description")
    ownership, home = ownership_for_tg(domain, peers)
    distribution, shared = distribution_for_tg(peers, tg)
    return {
        "tg": tg,
        "description": description,
        "ownership": ownership,
        "home": home,
        "distribution": distribution,
        "import_from": shared,
        "export_to": list(shared),
    }


def collect_talkgroups(domain: str,
                       peers: list[dict[str, Any]]) -> list[dict[str, Any]]:
    print("\nStep 3 of 3: Talkgroup policy")
    print("-" * 45)
    print("Unlisted talkgroups remain local by default. The builder creates exact")
    print("numeric routes, not wildcard or prefix routes.")
    talkgroups: list[dict[str, Any]] = []
    while True:
        route = ask_talkgroup(
            domain, peers, {int(item["tg"]) for item in talkgroups})
        if route is None:
            return talkgroups
        if route:
            talkgroups.append(route)


def normalise_plan(plan: dict[str, Any]) -> dict[str, Any]:
    result = copy.deepcopy(plan)
    reflector = result.get("reflector", {})
    if isinstance(reflector, dict):
        reflector["domain"] = str(reflector.get("domain", "")).upper()
        reflector["callsign"] = str(reflector.get("callsign", "")).upper()
    for peer in result.get("peers", []):
        if isinstance(peer, dict):
            peer["name"] = str(peer.get("name", "")).upper()
            peer["trust_callsign"] = str(peer.get("trust_callsign", "")).upper()
            peer["protocol"] = 2
    for route in result.get("talkgroups", []):
        if not isinstance(route, dict):
            continue
        route.pop("service_anchor", None)
        route.pop("allstar_node", None)
        shared = list(dict.fromkeys(route.get("import_from", [])))
        route["import_from"] = shared
        route["export_to"] = list(shared)
    return result


def validate_plan(plan: dict[str, Any]) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []
    if plan.get("plan_schema") != PLAN_SCHEMA:
        errors.append(f"plan_schema must be {PLAN_SCHEMA}")
    reflector = plan.get("reflector")
    if not isinstance(reflector, dict):
        return ["reflector must be an object"], warnings
    for field in ("domain", "reflector_id", "callsign", "library"):
        if not isinstance(reflector.get(field), str) or not reflector[field].strip():
            errors.append(f"reflector.{field} is required")
    if reflector.get("library") and not str(reflector["library"]).startswith("/"):
        warnings.append("reflector.library is normally an absolute path")

    peers = plan.get("peers", [])
    if not isinstance(peers, list):
        return errors + ["peers must be an array"], warnings
    peer_names: list[str] = []
    peer_ids: list[str] = []
    trust_ids: list[str] = []
    for index, peer in enumerate(peers):
        label = f"peers[{index}]"
        if not isinstance(peer, dict):
            errors.append(f"{label} must be an object")
            continue
        for field in ("name", "host", "reflector_id", "trust_callsign"):
            if not isinstance(peer.get(field), str) or not peer[field].strip():
                errors.append(f"{label}.{field} is required")
        name = str(peer.get("name", ""))
        peer_names.append(name)
        peer_ids.append(str(peer.get("reflector_id", "")))
        trust_ids.append(str(peer.get("trust_callsign", "")))
        port = peer.get("port")
        if not isinstance(port, int) or not 1 <= port <= 65535:
            errors.append(f"{label}.port must be between 1 and 65535")
        if peer.get("protocol") != 2:
            errors.append(f"{label}.protocol must currently be 2")
    for values, label in (
            (peer_names, "peer name"),
            (peer_ids, "peer reflector_id"),
            (trust_ids, "trust identity")):
        duplicates = sorted({item for item in values if item and values.count(item) > 1})
        if duplicates:
            errors.append(f"Duplicate {label}(s): {', '.join(duplicates)}")

    peer_set = set(peer_names)
    talkgroups = plan.get("talkgroups", [])
    if not isinstance(talkgroups, list):
        return errors + ["talkgroups must be an array"], warnings
    seen_tgs: set[int] = set()
    for index, route in enumerate(talkgroups):
        label = f"talkgroups[{index}]"
        if not isinstance(route, dict):
            errors.append(f"{label} must be an object")
            continue
        tg = route.get("tg")
        if not isinstance(tg, int) or tg <= 0:
            errors.append(f"{label}.tg must be a positive whole number")
        elif tg in seen_tgs:
            errors.append(f"Duplicate talkgroup: {tg}")
        else:
            seen_tgs.add(tg)
        if route.get("ownership") not in OWNERSHIP:
            errors.append(f"{label}.ownership is invalid")
        if route.get("distribution") not in DISTRIBUTION:
            errors.append(f"{label}.distribution is invalid")
        if not isinstance(route.get("home"), str) or not route["home"].strip():
            errors.append(f"{label}.home is required")
        imports = route.get("import_from", [])
        exports = route.get("export_to", [])
        if not isinstance(imports, list) or not isinstance(exports, list):
            errors.append(f"{label} peer permissions must be arrays")
            continue
        unknown = sorted((set(imports) | set(exports)) - peer_set)
        if unknown:
            errors.append(f"{label} contains unknown peers: {', '.join(unknown)}")
        if imports != exports:
            errors.append(f"{label} must use identical two-way import and export peers")
        if route.get("distribution") == "local" and imports:
            errors.append(f"{label} is local but has shared peers")
        if route.get("distribution") != "local" and not imports:
            errors.append(f"{label} is federated but has no shared peers")
        if route.get("ownership") == "peer" and route.get("home") not in peer_set:
            errors.append(f"{label}.home must name a configured peer")
    generation = plan.get("library_generation")
    if not isinstance(generation, int) or generation < 1:
        errors.append("library_generation must be a positive whole number")
    return errors, warnings


def print_messages(errors: list[str], warnings: list[str]) -> None:
    for warning in warnings:
        print(f"WARNING: {warning}")
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)


def federation_library(plan: dict[str, Any]) -> dict[str, Any]:
    policies: dict[str, dict[str, list[str]]] = {
        peer["name"]: {"import": [], "export": []}
        for peer in plan["peers"]
    }
    routes: list[dict[str, Any]] = []
    for route in plan["talkgroups"]:
        if route["distribution"] == "local":
            continue
        routes.append({
            "type": "exact",
            "value": route["tg"],
            "home": route["home"],
            "scope": "family",
            "description": route["description"],
        })
        value = str(route["tg"])
        for peer in route["import_from"]:
            policies[peer]["import"].append(value)
            policies[peer]["export"].append(value)
    for policy in policies.values():
        policy["import"] = sorted(set(policy["import"]), key=int)
        policy["export"] = sorted(set(policy["export"]), key=int)
    return {
        "schema": 1,
        "generation": plan["library_generation"],
        "domain": plan["reflector"]["domain"],
        "routes": sorted(routes, key=lambda item: item["value"]),
        "peer_policy": policies,
    }


def federation_conf(plan: dict[str, Any], keys: dict[str, str]) -> str:
    reflector = plan["reflector"]
    peers = plan["peers"]
    lines = [
        "###################################################################",
        "# Generated SVXReflector federation configuration",
        "# Review before installing as svxreflector.d/Federation.conf",
        "###################################################################", "", "[FEDERATION]",
        f"ENABLE={1 if reflector.get('enable') else 0}",
        f"DOMAIN={reflector['domain']}",
        f"REFLECTOR_ID={reflector['reflector_id']}",
        f"CALLSIGN={reflector['callsign']}",
        f"LIBRARY={reflector['library']}",
        "PEERS=" + ",".join(peer["name"] for peer in peers), "",
    ]
    for peer in peers:
        lines.extend([
            f"[FEDERATION_PEER_{peer['name']}]",
            f"HOST={peer['host']}",
            f"REFLECTOR_ID={peer['reflector_id']}",
            f"PORT={peer['port']}",
            f"PROTOCOL={peer['protocol']}",
            f"AUTH_KEY={keys[peer['name']]}",
            f"CONNECT={1 if peer.get('connect') else 0}", "",
        ])
    if peers:
        lines.append("[FEDERATION_TRUST]")
        lines.extend(f"{peer['trust_callsign']}={peer['name']}" for peer in peers)
        lines.append("")
    return "\n".join(lines)


def show_plan(plan: dict[str, Any]) -> None:
    reflector = plan["reflector"]
    print(f"\nDomain: {reflector['domain']}")
    print(f"Reflector: {reflector['reflector_id']}")
    print(f"Policy generation: {plan['library_generation']}")
    print_peer_summary(plan["peers"])
    print("\nTalkgroup policy:")
    if not plan["talkgroups"]:
        print("  None")
        return
    print("  TG       Description                    Home                 Shared with")
    for route in sorted(plan["talkgroups"], key=lambda item: item["tg"]):
        shared = ", ".join(route["import_from"]) or "LOCAL"
        description = str(route["description"])[:30]
        print(f"  {route['tg']:<8} {description:<30} "
              f"{route['home']:<20} {shared}")


def plan_diff(before: dict[str, Any], after: dict[str, Any]) -> str:
    return "".join(difflib.unified_diff(
        json_text(before).splitlines(True),
        json_text(after).splitlines(True),
        fromfile="current plan", tofile="proposed plan"))


def new_plan(path: Path) -> int:
    if path.exists():
        print(f"ERROR: Plan already exists: {path}", file=sys.stderr)
        print(f"Use 'edit {path}' to review or change it.", file=sys.stderr)
        print("No file was changed.", file=sys.stderr)
        return 1
    reflector = ask_reflector()
    peers = collect_peers()
    talkgroups = collect_talkgroups(str(reflector["domain"]), peers)
    plan = {
        "plan_schema": PLAN_SCHEMA,
        "library_generation": 1,
        "reflector": reflector,
        "peers": peers,
        "talkgroups": talkgroups,
    }
    errors, warnings = validate_plan(plan)
    print_messages(errors, warnings)
    if errors:
        print("Plan was not written because validation failed.", file=sys.stderr)
        return 1
    show_plan(plan)
    print("\nThis new plan uses federation policy generation 1.")
    if not ask_bool("Save this new plan", False):
        print("No file was written.")
        return 0
    write_json(path, plan)
    print(f"Plan written: {path}")
    return 0


def choose_talkgroup(plan: dict[str, Any]) -> dict[str, Any] | None:
    routes = sorted(plan["talkgroups"], key=lambda item: item["tg"])
    if not routes:
        print("No talkgroups are configured.")
        return None
    selected = choose("Select a talkgroup", [
        f"TG {route['tg']} - {route['description']}" for route in routes])
    return routes[selected]


def edit_talkgroups(plan: dict[str, Any]) -> None:
    labels = ["Add a talkgroup", "Remove a talkgroup", "Change shared peers",
              "Change description", "Change administrative ownership",
              "Return to main menu"]
    while True:
        action = choose("Talkgroup policy", labels, 6)
        if action == 5:
            return
        if action == 0:
            route = ask_talkgroup(
                plan["reflector"]["domain"], plan["peers"],
                {int(item["tg"]) for item in plan["talkgroups"]})
            if route:
                plan["talkgroups"].append(route)
            continue
        route = choose_talkgroup(plan)
        if route is None:
            continue
        if action == 1:
            if ask_bool(f"Remove TG {route['tg']} from this plan", False):
                plan["talkgroups"].remove(route)
        elif action == 2:
            distribution, shared = distribution_for_tg(plan["peers"], route["tg"])
            route["distribution"] = distribution
            route["import_from"] = shared
            route["export_to"] = list(shared)
        elif action == 3:
            route["description"] = ask("Talkgroup description", route["description"])
        elif action == 4:
            ownership, home = ownership_for_tg(
                plan["reflector"]["domain"], plan["peers"])
            route["ownership"] = ownership
            route["home"] = home


def peer_snippet(peer: dict[str, Any]) -> str:
    return "\n".join([
        f"# Merge {peer['name']} into PEERS in [FEDERATION]", "",
        f"[FEDERATION_PEER_{peer['name']}]",
        f"HOST={peer['host']}",
        f"REFLECTOR_ID={peer['reflector_id']}",
        f"PORT={peer['port']}", "PROTOCOL=2", "AUTH_KEY=CHANGE_ME",
        f"CONNECT={1 if peer.get('connect') else 0}", "",
        "# Merge under [FEDERATION_TRUST]",
        f"{peer['trust_callsign']}={peer['name']}", "",
    ])


def peer_removal(peer: dict[str, Any]) -> str:
    return "\n".join([
        f"Remove peer {peer['name']} from Federation.conf", "",
        f"1. Remove {peer['name']} from PEERS= in [FEDERATION].", "",
        f"2. Remove the complete [FEDERATION_PEER_{peer['name']}] section.", "",
        "3. Remove this line from [FEDERATION_TRUST]:",
        f"   {peer['trust_callsign']}={peer['name']}", "",
        "4. Review and install the revised federation policy.",
        "5. Restart SVXReflector during an agreed maintenance period.", "",
    ])


def edit_peers(plan: dict[str, Any], added: list[dict[str, Any]],
               removed: list[dict[str, Any]]) -> None:
    labels = ["Add a peer", "Remove a peer", "Change peer details",
              "Return to main menu"]
    while True:
        action = choose("Peer definitions", labels, 4)
        if action == 3:
            return
        if action == 0:
            peer = ask_peer({str(item["name"]) for item in plan["peers"]})
            if peer is not None:
                plan["peers"].append(peer)
                added.append(peer)
                print("No talkgroups are shared with the new peer automatically.")
            continue
        if not plan["peers"]:
            print("No peers are configured.")
            continue
        index = choose("Select a peer", [str(peer["name"]) for peer in plan["peers"]])
        peer = plan["peers"][index]
        if action == 1:
            affected = [route for route in plan["talkgroups"]
                        if peer["name"] in route["import_from"]]
            if affected:
                print("\nAffected talkgroups:")
                for route in affected:
                    print(f"  TG {route['tg']} - {route['description']}")
            if not ask_bool(f"Remove peer {peer['name']} and these permissions", False):
                continue
            for route in affected:
                route["import_from"].remove(peer["name"])
                route["export_to"].remove(peer["name"])
                if not route["import_from"]:
                    route["distribution"] = "local"
            plan["peers"].remove(peer)
            removed.append(copy.deepcopy(peer))
        elif action == 2:
            peer["host"] = ask("Remote federation service hostname", peer["host"])
            peer["reflector_id"] = peer["host"]
            peer["port"] = ask_int("Remote service port", peer["port"], 1, 65535)
            peer["connect"] = ask_bool(
                f"Maintain an outgoing connection to {peer['name']}",
                bool(peer.get("connect")))
            peer["trust_callsign"] = ask(
                f"Incoming identity used by {peer['name']}",
                peer["trust_callsign"]).upper()
            if peer not in added:
                added.append(copy.deepcopy(peer))


def edit_reflector(plan: dict[str, Any]) -> None:
    reflector = plan["reflector"]
    print("\nChanging these values requires corresponding changes on affected peers.")
    reflector["domain"] = ask("Federation domain", reflector["domain"]).upper()
    reflector["reflector_id"] = ask(
        "Federation service hostname", reflector["reflector_id"])
    reflector["callsign"] = ask(
        "Outgoing federation identity", reflector["callsign"]).upper()
    reflector["library"] = ask("Federation policy file", reflector["library"])
    reflector["enable"] = ask_bool(
        "Enable federation in generated configuration",
        bool(reflector.get("enable")))
    for route in plan["talkgroups"]:
        if route["ownership"] == "domain":
            route["home"] = reflector["domain"]


def save_edited_plan(path: Path, before: dict[str, Any], after: dict[str, Any],
                     added: list[dict[str, Any]],
                     removed: list[dict[str, Any]], output: Path) -> int:
    after = normalise_plan(after)
    if after == before:
        print("No changes were made.")
        return 0
    after["library_generation"] = int(before["library_generation"]) + 1
    errors, warnings = validate_plan(after)
    print_messages(errors, warnings)
    if errors:
        print("Changes were not saved.", file=sys.stderr)
        return 1
    print("\nProposed changes:\n")
    print(plan_diff(before, after))
    show_plan(after)
    print(f"\nPolicy generation will advance from "
          f"{before['library_generation']} to {after['library_generation']}.")
    if not ask_bool("Save these changes and generate a candidate policy", False):
        print("No file was changed.")
        return 0

    stamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    backup = path.with_name(path.name + f".{stamp}.bak")
    write_text(backup, json_text(before), 0o644)
    write_json(path, after, replace=True)
    domain_dir = output / safe_name(after["reflector"]["domain"])
    candidate = domain_dir / (
        f"federation-generation-{after['library_generation']}.json")
    write_json(candidate, federation_library(after))
    for peer in added:
        snippet = domain_dir / f"peer-{safe_name(peer['name'])}.conf"
        write_text(snippet, peer_snippet(peer), 0o600)
    for peer in removed:
        checklist = domain_dir / f"remove-peer-{safe_name(peer['name'])}.txt"
        write_text(checklist, peer_removal(peer), 0o600)
    print(f"\nPlan saved: {path}")
    print(f"Previous plan: {backup}")
    print(f"Candidate policy: {candidate}")
    print("No installed files were changed.")
    return 0


def edit_plan(path: Path, output: Path) -> int:
    try:
        original = normalise_plan(load_plan(path))
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    errors, warnings = validate_plan(original)
    print_messages(errors, warnings)
    if errors:
        return 1
    proposed = copy.deepcopy(original)
    added: list[dict[str, Any]] = []
    removed: list[dict[str, Any]] = []
    labels = ["Talkgroup policy", "Peer definitions", "Local reflector details",
              "Review proposed plan", "Save and generate", "Exit without changes"]
    while True:
        show_plan(proposed)
        action = choose("Edit federation plan", labels, 4)
        if action == 0:
            edit_talkgroups(proposed)
        elif action == 1:
            edit_peers(proposed, added, removed)
        elif action == 2:
            edit_reflector(proposed)
        elif action == 3:
            print("\n" + (plan_diff(original, proposed) or "No changes."))
        elif action == 4:
            return save_edited_plan(path, original, proposed, added, removed, output)
        else:
            print("No file was changed.")
            return 0


def check(path: Path) -> int:
    try:
        plan = load_plan(path)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    errors, warnings = validate_plan(plan)
    print_messages(errors, warnings)
    if errors:
        return 1
    print(f"Plan is valid: {path}")
    print(f"Peers: {len(plan['peers'])}; talkgroups: {len(plan['talkgroups'])}")
    print(f"Federated routes emitted: {len(federation_library(plan)['routes'])}")
    return 0


def show(path: Path) -> int:
    try:
        plan = load_plan(path)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    errors, warnings = validate_plan(plan)
    print_messages(errors, warnings)
    if errors:
        return 1
    show_plan(plan)
    return 0


def build(path: Path, output: Path, placeholders: bool) -> int:
    try:
        plan = load_plan(path)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    errors, warnings = validate_plan(plan)
    print_messages(errors, warnings)
    if errors:
        return 1
    keys: dict[str, str] = {}
    for peer in plan["peers"]:
        if placeholders:
            keys[peer["name"]] = "CHANGE_ME"
        else:
            key = getpass.getpass(
                f"AUTH_KEY for outgoing connection to {peer['name']} "
                "(blank writes CHANGE_ME): ")
            keys[peer["name"]] = key or "CHANGE_ME"
    domain_dir = output / safe_name(plan["reflector"]["domain"])
    conf = domain_dir / "Federation.conf"
    library = domain_dir / "federation.json"
    try:
        if conf.exists() or library.exists():
            existing = conf if conf.exists() else library
            raise FileExistsError(f"File already exists: {existing}")
        write_text(conf, federation_conf(plan, keys), 0o600)
        write_json(library, federation_library(plan))
    except FileExistsError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        print("Use a separate output directory. No existing file was replaced.",
              file=sys.stderr)
        return 1
    print(f"Generated: {conf}")
    print(f"Generated: {library}")
    if any(value == "CHANGE_ME" for value in keys.values()):
        print("WARNING: Federation.conf contains CHANGE_ME placeholders.")
    print("No installed files were changed.")
    return 0


def build_library(path: Path, output: Path) -> int:
    try:
        plan = load_plan(path)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    errors, warnings = validate_plan(plan)
    print_messages(errors, warnings)
    if errors:
        return 1
    candidate = output / safe_name(plan["reflector"]["domain"]) / (
        f"federation-generation-{plan['library_generation']}.json")
    try:
        write_json(candidate, federation_library(plan))
    except FileExistsError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        print("No existing file was replaced.", file=sys.stderr)
        return 1
    print(f"Candidate policy: {candidate}")
    print("No installed files were changed.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Plan and generate native SVXReflector federation configuration")
    sub = parser.add_subparsers(dest="command", required=True)
    new_parser = sub.add_parser("new", help="create a new per-reflector plan")
    new_parser.add_argument("plan", type=Path)
    edit_parser = sub.add_parser("edit", help="review and change an existing plan")
    edit_parser.add_argument("plan", type=Path)
    edit_parser.add_argument("--output", type=Path, default=Path("generated"))
    show_parser = sub.add_parser("show", help="display a readable plan summary")
    show_parser.add_argument("plan", type=Path)
    check_parser = sub.add_parser("check", help="validate an existing plan")
    check_parser.add_argument("plan", type=Path)
    build_parser = sub.add_parser(
        "build", help="create initial Federation.conf and federation.json")
    build_parser.add_argument("plan", type=Path)
    build_parser.add_argument("--output", type=Path, default=Path("generated"))
    build_parser.add_argument(
        "--placeholders", action="store_true",
        help="write CHANGE_ME instead of requesting authentication keys")
    library_parser = sub.add_parser(
        "library", help="generate a versioned federation policy candidate")
    library_parser.add_argument("plan", type=Path)
    library_parser.add_argument("--output", type=Path, default=Path("generated"))
    args = parser.parse_args()
    if args.command == "new":
        return new_plan(args.plan)
    if args.command == "edit":
        return edit_plan(args.plan, args.output)
    if args.command == "show":
        return show(args.plan)
    if args.command == "check":
        return check(args.plan)
    if args.command == "build":
        return build(args.plan, args.output, args.placeholders)
    if args.command == "library":
        return build_library(args.plan, args.output)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
