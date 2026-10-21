# SVXReflector Federation

## Status

SVXReflector federation is implemented as an integrated component of each
participating SVXReflector. The current implementation uses the established
V2-compatible reflector transport for authentication and framing, with a
separate federation application protocol carried over that connection.

The implementation has been exercised between independently configured
SVXReflectors in both directions, including authentication, federation session
establishment, UDP registration, stream acceptance, OPUS audio forwarding,
normal stream termination and cleanup after an originating client disconnect.

The protocol and configuration should still be treated as development
interfaces until they have received broader interoperability testing.

## Provenance

This work is based solely on the official SvxLink/SVXReflector source by
Tobias Blomberg, SM0SVX, and the observed behaviour required by the G4NAB
reflector-family deployment.

GeuReflector is explicitly excluded as a source or technical reference. Its
source code, architecture and implementation details have not been consulted,
copied, translated or adapted.

## Purpose

Federation provides controlled, distributed, on-demand talkgroup routing
between autonomous SVXReflectors without an independent central federation
server and without client-side ReflectorLogic bridges between every server.

Each reflector remains locally administered. A talkgroup is exchanged with a
peer only when that reflector's own configuration and policy explicitly permit
it.

## Implemented principles

- Federation is integrated into every participating SVXReflector.
- There is no central federation process or mandatory hub.
- Peer relationships, credentials and policy are configured locally.
- Each talkgroup has independent stream and talker state.
- Different talkgroups may be active simultaneously.
- Only OPUS audio is federated.
- Encoded OPUS frames are forwarded without transcoding.
- Imported audio is delivered locally but is never re-exported.
- A locally originated stream is sent directly to every connected peer for
  which export policy permits that talkgroup.
- Import and export are independently denied by default.
- Failure or restart of a peer causes its connector to disconnect and retry.
- A peer disconnect removes its incoming streams and releases local listener
  state.

## Architecture

### `ReflectorFederation`

`ReflectorFederation` owns the local federation configuration, policy library,
trusted incoming sessions, incoming stream state, outgoing peer connectors and
locally originated stream lifecycle.

It coordinates stream start, audio and stop operations across every eligible
outgoing peer. It also reports incoming stream start and stop events to
`Reflector`, allowing ordinary local clients to receive the normal talker and
flush notifications.

### `FederationPeerConnection`

`FederationPeerConnection` represents one locally initiated connection to a
configured peer. It performs:

- V2-compatible challenge-response authentication;
- federation hello and capability negotiation;
- UDP path registration;
- TCP and UDP heartbeat maintenance;
- reconnect after connection failure;
- outgoing stream request, result and stop handling;
- outgoing OPUS audio sequencing;
- bounded audio buffering while stream acceptance is pending.

Only peers with `CONNECT=1` have a locally initiated connector.

### `FederationLibrary`

`FederationLibrary` loads and validates the JSON talkgroup and peer-policy
library. The library provides route classification and explicit per-peer import
and export decisions.

The library is read during reflector initialisation. Runtime hot reload,
rollback and route advertisement are not part of the current implementation.

## Connection model

An outgoing connector is directional. `CONNECT=1` means that the local
reflector initiates and maintains a connection to that peer. `CONNECT=0` means
that no local connector is created, although a correctly authenticated and
trusted incoming session from that peer may still be accepted.

Bidirectional federation normally uses reciprocal configuration:

1. Reflector A configures Reflector B with `CONNECT=1`.
2. Reflector B configures Reflector A with `CONNECT=1`.
3. A establishes an authenticated outgoing session to B.
4. B independently establishes an authenticated outgoing session to A.

There are therefore two directional sessions between a fully bidirectional
pair. Each direction has its own authentication, hello negotiation, UDP
registration, heartbeat state and reconnect behaviour.

Restarting either reflector is an ordinary operational event. The surviving
connector detects the lost session and retries until its peer is available
again.

## Federation access identities

Each reflector has one stable federation access identity configured by
`CALLSIGN`. The name is retained because authentication uses the existing V2
callsign/password mechanism, but the value may be an administrator-issued
service identity rather than an on-air amateur callsign.

A consistent family allocation is recommended:

```text
REFLECTOR-FUK    UK-WIDE
REFLECTOR-FNA    NORTH-AMERICA
REFLECTOR-FYK    YORKSHIRENET
REFLECTOR-FAU    AUSTRALIA
```

The access identity:

- must be unique within the federation family;
- remains stable when a hostname or address changes;
- is authorised through the receiving reflector's normal user/password
  configuration;
- must be accepted by the receiving reflector's configured callsign rules;
- is mapped to exactly one peer through `[FEDERATION_TRUST]`.

The stable `REFLECTOR_ID` is a separate identity. It identifies the reflector
in federation messages and should normally be a globally unique DNS-style
name.

## Federation access registration and trust

Each participating reflector uses a dedicated federation access identity, for
example `REFLECTOR-FUK` or `REFLECTOR-FNA`.

Before federation can operate, the administrator of each receiving reflector
must register and accept the remote reflector's federation access identity
through the normal SVXReflector callsign and X.509 certificate administration
process.

This is an administrative prerequisite. It does not change talkgroup handling,
UDP transport, OPUS audio, stream identity or routing policy.

After the federation access identity has been accepted:

1. The identity must appear in `[FEDERATION_TRUST]`.
2. The trust entry maps that identity to exactly one configured peer.
3. The federation hello must contain the expected `REFLECTOR_ID` and domain.
4. Only then is the authenticated connection granted federation privileges.

An accepted ordinary client identity has no federation privileges unless it is
also explicitly mapped in `[FEDERATION_TRUST]`.

The federation access identity identifies the peer reflector. It is separate
from the callsign of the user originating a talkgroup stream. For example,
`REFLECTOR-FNA` may carry a stream whose source callsign is `G4NAB-10`.

## Transport and wire protocol

Federation control messages use the authenticated TCP connection. Federation
OPUS frames use its associated registered UDP path.

The federation application protocol has its own version and capability flags,
independent of the underlying reflector transport version. The current hello
negotiation uses federation protocol version 1.0.

### Control messages

| Type | Message | Purpose |
| ---: | --- | --- |
| 200 | `MsgFederationHello` | Present reflector identity, domain, library generation and capabilities |
| 201 | `MsgFederationHelloAck` | Accept the federation session and negotiated capabilities |
| 202 | `MsgFederationStreamStart` | Request an outgoing talkgroup stream |
| 203 | `MsgFederationStreamResult` | Accept or reject the requested stream |
| 204 | `MsgFederationStreamStop` | End an established or pending stream |

### UDP audio

`MsgUdpFederationAudio` identifies every audio frame by:

- originating reflector ID;
- talkgroup;
- stream ID;
- audio sequence number;
- encoded OPUS payload.

Federation audio is distinct from ordinary client `MsgUdpAudio`. A receiving
reflector validates the federation session, stream identity and sequence
before converting the payload into ordinary local reflector audio for its
listeners.

### Stream identity

A federation stream is identified by:

```text
origin_reflector_id
talkgroup
stream_id
```

The locally generated stream ID distinguishes successive streams on the same
talkgroup. The origin identity prevents a peer from presenting a stream as if
it came from another configured reflector.

## Configuration

Federation configuration is kept in `Federation.conf`, normally installed from
`Federation.conf.in`. Its directory is included through `GLOBAL/CFG_DIR` in
the main SVXReflector configuration.

Federation is inactive unless `[FEDERATION]` contains `ENABLE=1`.

### Local configuration

```ini
[FEDERATION]
ENABLE=1
DOMAIN=UK-WIDE
REFLECTOR_ID=uk-wide.example.org
CALLSIGN=REFLECTOR-FUK
LIBRARY=/etc/svxlink/federation.json
PEERS=NORTH-AMERICA

[FEDERATION_PEER_NORTH-AMERICA]
HOST=north-america.example.org
REFLECTOR_ID=north-america.example.org
PORT=35300
PROTOCOL=2
AUTH_KEY=replace-with-the-remote-access-key
CONNECT=1

[FEDERATION_TRUST]
REFLECTOR-FNA=NORTH-AMERICA
```

### `[FEDERATION]` values

| Value | Meaning |
| --- | --- |
| `ENABLE` | Enables federation when set to `1` |
| `DOMAIN` | Administrative federation domain for the local reflector |
| `REFLECTOR_ID` | Stable and unique local reflector identity |
| `CALLSIGN` | Local federation access identity used for outgoing authentication |
| `LIBRARY` | Path to the local JSON route and policy library |
| `PEERS` | Comma-separated configured peer names |

### Peer values

Each entry in `PEERS` requires a matching
`[FEDERATION_PEER_<name>]` section.

| Value | Meaning |
| --- | --- |
| `HOST` | Remote hostname or address |
| `REFLECTOR_ID` | Expected stable identity of the remote reflector; defaults to `HOST` when omitted |
| `PORT` | Remote reflector service port |
| `PROTOCOL` | Underlying reflector protocol used by the outgoing connector; currently `2` |
| `AUTH_KEY` | Password used with the local federation access identity on the remote reflector |
| `CONNECT` | Creates and maintains a local outgoing connector when set to `1` |

Peer names are policy identifiers and must match consistently between
`PEERS`, peer section names, `[FEDERATION_TRUST]` values and JSON
`peer_policy` keys.

### Incoming trust

`[FEDERATION_TRUST]` maps a remote authenticated access identity to its local
peer name:

```ini
[FEDERATION_TRUST]
REFLECTOR-FNA=NORTH-AMERICA
```

Each configured peer requires exactly one trusted incoming identity. The same
identity must also be authorised through the ordinary reflector access
configuration.

## Talkgroup library

The JSON library contains route metadata and explicit peer policy. It contains
no passwords or private keys.

```json
{
  "schema": 1,
  "generation": 3,
  "domain": "UK-WIDE",
  "routes": [
    {
      "type": "exact",
      "value": 235,
      "home": "UK-WIDE",
      "scope": "family",
      "description": "UK-wide calling"
    },
    {
      "type": "exact",
      "value": 9050,
      "home": "UK-WIDE",
      "scope": "family",
      "description": "Example shared gateway talkgroup"
    }
  ],
  "peer_policy": {
    "NORTH-AMERICA": {
      "import": ["235"],
      "export": ["235", "9050"]
    }
  }
}
```

### Routes

Only a configured route with `scope` set to `family` is eligible for
federation. A missing route, an unsupported route or any other scope leaves
the talkgroup local.

The `home` value records administrative ownership or routing metadata. It does
not by itself authorise a peer to originate the talkgroup. This permits a
family talkgroup to carry replies originating at another authorised
reflector.

### Peer policy

Every peer has independent `import` and `export` lists:

- `import` permits locally receiving a matching talkgroup from that peer;
- `export` permits sending a locally originated matching talkgroup to that
  peer.

Entries may be exact talkgroup numbers such as `235` or supported prefix
patterns such as `310*`. No match means deny.

Policy is evaluated locally. Authentication, trust, a family-scoped route and
the appropriate peer-policy match are all required; none of them alone grants
permission.

The library generation is announced during federation hello negotiation for
diagnostic and compatibility purposes. Libraries are not advertised or
remotely installed by the current implementation.

## Federation configuration builder

`svx_federation_builder.py` is installed with SVXReflector as an interactive
planning and policy administration tool. It creates a separate planning JSON
for each reflector and generates the corresponding `Federation.conf` and
federation policy library.

The planning JSON is the administrator's editable source of truth. It contains
reflector identities, peer definitions, talkgroup ownership and two-way sharing
decisions, but deliberately contains no authentication keys.

The builder never installs files into `/etc`, edits an installed
`Federation.conf`, or restarts SVXReflector.

The supported commands are:

```text
svx_federation_builder.py new PLAN
svx_federation_builder.py edit PLAN [--output DIRECTORY]
svx_federation_builder.py show PLAN
svx_federation_builder.py check PLAN
svx_federation_builder.py build PLAN [--output DIRECTORY] [--placeholders]
svx_federation_builder.py library PLAN [--output DIRECTORY]
```

### Administrative model

Every reflector has its own plan and evaluates policy locally. Peer
relationships are explicitly bilateral: technical reachability or membership
of the same network family does not grant permission to federate.

Adding a peer to one reflector does not add it to any other reflector. A
manager should enter peer information only after the two participating
managers have agreed the direct relationship and the information may be used
for that deployment.

Talkgroups shared through this builder are always two-way. A selected peer is
added to both the runtime `import` and `export` policy for the talkgroup. The
corresponding peer must independently configure the reciprocal permission.

Federation remains non-transitive. Imported streams are never re-exported, so
two reflectors that need to exchange a talkgroup must have an approved direct
relationship. A genuinely family-wide talkgroup requires direct relationships
between every pair of participating reflectors expected to hear one another.

### Create a new reflector plan

Run the staged questionnaire once for each reflector:

```sh
svx_federation_builder.py new GREAT-BRITAIN-plan.json
```

The `new` command refuses to overwrite an existing plan. If the named file
already exists, use `edit` instead.

The questionnaire has three stages:

1. local reflector identity;
2. approved direct federation peers;
3. talkgroup policy.

Administrative identifiers such as the domain, federation login identities
and peer names are converted to uppercase. DNS hostnames and paths retain the
case entered by the administrator.

#### Local reflector identity

The local questions record:

- the administrative federation domain, for example `GREAT-BRITAIN`;
- the permanent federation service DNS hostname;
- the dedicated outgoing federation login identity;
- the final installed policy path;
- the initial enabled state for a generated configuration.

The federation service hostname is used as the local `REFLECTOR_ID`. It is the
hostname of the SVXReflector service, not a public website URL. For example, a
reflector may use `uk.wide.svxlink.uk` as its federation service hostname while
publishing a dashboard at a different web address.

The outgoing federation identity is a reflector service account, not the
callsign of an amateur originating talkgroup audio. An example Great Britain
identity is `REFLECTOR-FGB`. It must be registered and accepted by every
receiving reflector and assigned to the correct peer in that reflector's
`[FEDERATION_TRUST]` section.

The standard policy installation path is:

```text
/etc/svxlink/federation.json
```

The initial safety default is `ENABLE=0`. The setting takes effect only after
the generated file is reviewed, installed and SVXReflector is restarted.

#### Direct peers

A peer is another approved federation-enabled SVXReflector with which this
reflector may exchange traffic directly. Before accepting a new peer, the
questionnaire asks the administrator to confirm that the remote manager has
approved the relationship.

For each peer, the plan records:

- an uppercase policy name such as `NORTH-AMERICA`;
- the remote federation service DNS hostname;
- the remote TCP/UDP service port;
- whether this reflector maintains an outgoing connector;
- the incoming federation login identity used by that peer.

The remote federation service hostname is written as both `HOST` and the
expected remote `REFLECTOR_ID`. It must not be a website URL or include an
`https://` prefix or page path.

Federation connections currently use the established protocol 2 callsign and
password authentication path. The builder records `PROTOCOL=2` without asking
the administrator to select an unsupported alternative. This does not remove
the protocol 3 and X.509 capabilities of the installed SVXReflector software.

For bidirectional transport, each reflector normally maintains its own
outgoing connection to the other. `CONNECT=0` accepts the approved incoming
relationship without starting a local outgoing connector.

The incoming login identity is the service account presented by the remote
reflector when it connects locally. It is mapped to the peer name under
`[FEDERATION_TRUST]` and must already be registered and authenticated locally.

The completed peer list is displayed for confirmation before talkgroup entry
begins. Talkgroup ownership and sharing choices use this fixed list.

#### Talkgroup policy

Unlisted talkgroups remain local by default and cannot be imported or
exported. An administrator may also record a talkgroup explicitly as local;
it remains in the plan but is not emitted as a federation route.

The builder creates exact positive integer routes. Prefix routes supported by
the runtime library must currently be administered separately.

Each entered talkgroup records:

- its number and human-readable description;
- administrative ownership and home;
- its distribution class;
- the approved peers sharing it in both directions.

Administrative ownership has three choices:

- `domain`: administered by the local federation domain;
- `peer`: administered by one configured remote peer;
- `network-wide`: a family service not assigned to one regional domain.

For domain-owned and network-wide talkgroups, the administrative home is
assigned automatically. For peer-owned talkgroups, the administrator selects
the responsible peer from the configured list.

Ownership does not grant transport permission. For example, TG 310 may be
peer-owned by `NORTH-AMERICA` in the Great Britain plan while still permitting
Great Britain users to participate through an approved two-way policy.

Distribution has three administrator-facing choices:

- local only;
- selected approved peers;
- all approved peers in this plan.

The internal value for the third choice remains `network-wide`. It means all
peers explicitly configured and approved in this local plan, not every current
or future member of the wider network family. A newly added peer is never
inserted into existing talkgroup policy automatically.

The same selected peer list is stored in `import_from` and `export_to`.
Validation rejects a manually edited plan whose two lists differ.

### Planning JSON structure

A new plan starts at generation 1. Its overall structure is:

```json
{
  "plan_schema": 1,
  "library_generation": 1,
  "reflector": {
    "domain": "GREAT-BRITAIN",
    "reflector_id": "uk.wide.svxlink.uk",
    "callsign": "REFLECTOR-FGB",
    "library": "/etc/svxlink/federation.json",
    "enable": false
  },
  "peers": [
    {
      "name": "NORTH-AMERICA",
      "host": "na.example.org",
      "reflector_id": "na.example.org",
      "port": 35300,
      "protocol": 2,
      "connect": true,
      "trust_callsign": "REFLECTOR-FNA"
    }
  ],
  "talkgroups": [
    {
      "tg": 235,
      "description": "UK-wide calling",
      "ownership": "domain",
      "home": "GREAT-BRITAIN",
      "distribution": "selected-peers",
      "import_from": [
        "NORTH-AMERICA"
      ],
      "export_to": [
        "NORTH-AMERICA"
      ]
    }
  ]
}
```

The example identities and hostnames illustrate the format only. Actual peer
information must be agreed by the participating managers. Authentication keys
must never be added to the planning JSON.

### Display and validate a plan

Display a readable summary without changing any file:

```sh
svx_federation_builder.py show GREAT-BRITAIN-plan.json
```

The summary lists the domain, reflector, generation, peers, talkgroups,
administrative homes and two-way sharing relationships.

Validate a plan after creation or manual editing:

```sh
svx_federation_builder.py check GREAT-BRITAIN-plan.json
```

Validation rejects missing identities, duplicate peers or talkgroups, invalid
ports, unknown peer references, asymmetric import/export policy, local
talkgroups with peer permissions and peer-owned talkgroups whose home is not a
configured peer.

### Create the initial deployment files

Generate an initial configuration and runtime library:

```sh
svx_federation_builder.py build \
  GREAT-BRITAIN-plan.json \
  --output generated
```

For each outgoing peer, the builder privately requests its `AUTH_KEY` through
a non-echoing terminal prompt. Keys are written only to the generated
`Federation.conf` and never to the plan.

For unattended preparation, placeholders may be requested explicitly:

```sh
svx_federation_builder.py build \
  GREAT-BRITAIN-plan.json \
  --output generated \
  --placeholders
```

This writes `CHANGE_ME` for every key. Such a configuration is not ready for
deployment.

For a `GREAT-BRITAIN` domain, initial output is:

```text
generated/GREAT-BRITAIN/Federation.conf
generated/GREAT-BRITAIN/federation.json
```

`Federation.conf` is created with mode `0600`; `federation.json` is created
with mode `0644`. `build` refuses to replace either existing output file and
instructs the administrator to select another output directory.

### Review and edit an existing plan

Open the interactive editor with:

```sh
svx_federation_builder.py edit \
  GREAT-BRITAIN-plan.json \
  --output generated
```

The editor first presents the current reflector, peer and talkgroup policy. It
provides separate menus for:

- adding, removing or changing talkgroups;
- changing the approved peers sharing a talkgroup;
- adding, removing or changing peer definitions;
- reviewing local reflector details;
- displaying the proposed JSON changes;
- saving or exiting without changes.

Adding a peer never shares existing talkgroups with it automatically. The
administrator must add the new peer deliberately to each agreed talkgroup.

Removing a peer lists every affected talkgroup before confirmation and removes
that peer from their two-way sharing lists. If a talkgroup has no remaining
peer, it becomes local.

Before saving, the builder validates the proposed plan, displays a unified
diff and asks for final confirmation. On approval it:

1. writes a timestamped backup beside the existing plan;
2. increments `library_generation` automatically;
3. atomically replaces the planning JSON;
4. creates a generation-numbered candidate policy;
5. leaves all installed files unchanged.

Example output for generation 4 is:

```text
GREAT-BRITAIN-plan.json.20260819-104500.bak
generated/GREAT-BRITAIN/federation-generation-4.json
```

If a peer was added or changed, the editor also writes a mode `0600`
peer-specific configuration snippet such as:

```text
generated/GREAT-BRITAIN/peer-AUSTRALIA.conf
```

The administrator merges this snippet into the installed `Federation.conf`
and supplies the correct `AUTH_KEY`. Existing credentials are never collected
or rewritten by the editor.

If a peer was removed, the editor writes a removal checklist such as:

```text
generated/GREAT-BRITAIN/remove-peer-AUSTRALIA.txt
```

It identifies the `PEERS` entry, peer section and trust mapping that must be
removed manually. The builder never edits the installed configuration.

### Generate a versioned policy candidate

Generate a policy from an unchanged valid plan without producing
`Federation.conf`:

```sh
svx_federation_builder.py library \
  GREAT-BRITAIN-plan.json \
  --output generated
```

For generation 4 this creates:

```text
generated/GREAT-BRITAIN/federation-generation-4.json
```

The command refuses to replace an existing candidate of the same generation.
Increment the generation only when making a deliberate policy revision.

### Review and install generated files

Inspect a candidate before installation:

```sh
python3 -m json.tool \
  generated/GREAT-BRITAIN/federation-generation-4.json

diff -u \
  /etc/svxlink/federation.json \
  generated/GREAT-BRITAIN/federation-generation-4.json
```

For an initial deployment, inspect the generated configuration and modes:

```sh
sed -n '1,240p' \
  generated/GREAT-BRITAIN/Federation.conf

stat -c '%a %n' \
  generated/GREAT-BRITAIN/Federation.conf \
  generated/GREAT-BRITAIN/federation.json
```

Confirm that all identities, hostnames, ports, trust mappings, two-way
talkgroup permissions and authentication keys are correct. No `CHANGE_ME`
value may remain in a deployed configuration.

Install reviewed files deliberately using the required permissions. For a
policy-only revision:

```sh
sudo install \
  -o root \
  -g root \
  -m 644 \
  generated/GREAT-BRITAIN/federation-generation-4.json \
  /etc/svxlink/federation.json
```

For an initial configuration:

```sh
sudo install \
  -o root \
  -g root \
  -m 600 \
  generated/GREAT-BRITAIN/Federation.conf \
  /etc/svxlink/svxreflector.d/Federation.conf
```

Do not restart a production reflector until reciprocal authentication, trust
and talkgroup policy have been agreed and prepared at every affected peer.

Use a separate plan for every reflector. Planning files may be retained in a
private administrative repository when appropriate. Generated
`Federation.conf` files, peer snippets and authentication keys must never be
committed to the SvxLink source repository.

## Locally originated streams

Ordinary client audio follows the existing SVXReflector talker arbitration.
Only the accepted local talker can originate a federation stream.

When the first accepted OPUS frame arrives on a talkgroup:

1. `Reflector` confirms that the talkgroup is not occupied by an incoming
   federation stream.
2. `ReflectorFederation` creates one local stream with a non-zero stream ID.
3. It evaluates every locally configured outgoing connector.
4. It selects only peers whose connector is connected, whose UDP path is
   registered and whose export policy permits the talkgroup.
5. It sends an independent stream-start request to each selected peer.
6. The local stream is recorded once even when no peer is currently eligible,
   preventing repeated start attempts for every audio frame.


### Pending audio

Audio may arrive before a peer accepts its stream-start request. Each outgoing
peer stream therefore has a bounded pending queue:

- up to 25 encoded OPUS frames are retained;
- when full, the oldest frame is discarded;
- sequence numbers are assigned only when frames are transmitted;
- queued frames are sent in order immediately after acceptance;
- rejection or disconnect removes the corresponding outgoing stream state.

Once active, subsequent frames are sent immediately through the registered UDP
path.

### Local stream termination

The local stream ends when the existing reflector talker state ends. This
includes:

- an ordinary client flush;
- talker audio timeout;
- SQL timeout handling;
- originating client disconnect.

`ReflectorFederation` sends a stream-stop message to every matching outgoing
peer and removes the local stream state. Cleanup is tied to the central talker
transition, so the different termination paths share the same behaviour.

## Incoming streams

An incoming stream request is accepted only when:

- federation is enabled;
- the session belongs to a configured and trusted peer;
- the origin reflector ID matches that peer;
- the talkgroup and stream ID are non-zero;
- the source identity is present;
- the codec is `OPUS`;
- the talkgroup has a `family` route;
- local import policy permits that peer and talkgroup;
- the talkgroup does not already contain another incoming federation stream.

After acceptance, UDP frames must match the peer, origin, talkgroup and stream
identity. Sequence state is tracked independently for each talkgroup.

The receiving reflector:

1. announces the imported source to applicable V2 local clients with
   `MsgTalkerStart`;
2. broadcasts decoded federation payloads as ordinary encoded
   `MsgUdpAudio` frames to local listeners on that talkgroup;
3. never passes those imported frames into the outgoing local-stream path;
4. announces `MsgTalkerStop` when the stream ends;
5. sends `MsgUdpFlushSamples` to local listeners so buffered audio is released.

If the peer session disconnects, all incoming streams owned by that peer are
removed and the same local stop and flush lifecycle is emitted.

## Loop prevention and direct distribution

Loop prevention follows one simple boundary: only accepted ordinary local
client audio may enter the outgoing federation lifecycle.

`MsgUdpFederationAudio` is validated and delivered directly to local clients.
It is not converted into a new locally originated federation stream. Imported
audio therefore cannot be reflected back to its source or forwarded to a
third peer.

For a shared talkgroup across several reflectors, each possible origin must
have direct export permission and connectivity to every intended recipient.
A reflector does not act as a transit hub for another reflector's imported
stream.

A reply from a user on a receiving reflector is a new locally originated
stream. It receives a new origin reflector ID and stream ID and is distributed
according to that reflector's own export policy.

## Operational behaviour

### Startup

When federation is enabled, SVXReflector validates the static configuration,
trust mappings and JSON library during initialisation. It creates outgoing
connectors only for peers with `CONNECT=1` and then starts their asynchronous
connection attempts.

The startup log reports local identity, policy generation, route count, peer
configuration, trust mappings and outgoing connector count.

### Reconnection

Outgoing connectors reconnect after refusal, peer restart, transport failure
or heartbeat timeout. Authentication, hello negotiation and UDP registration
are repeated for every new session.

Runtime streams are session state. They are not restored across a disconnect;
the ordinary talker must establish a later stream if audio continues after
connectivity returns.

### Diagnostics

Normal logs identify:

- TCP connection and authentication;
- federation hello acceptance;
- UDP path registration;
- outgoing stream request, acceptance, rejection and stop;
- incoming stream acceptance and stop;
- disconnect and reconnect causes.

Credentials and authentication keys must not be written to logs.

## Tested behaviour

The development test environment has verified:

- default-denied export policy;
- disconnected peers receiving no stream request;
- accepted peer authentication and federation hello negotiation;
- confirmed UDP registration and heartbeats;
- outgoing stream pending and active states;
- OPUS audio queued before acceptance and flushed afterward;
- normal local stream stop;
- local stream cleanup after an originating client disconnect;
- direct UK-WIDE to NORTH-AMERICA audio;
- direct NORTH-AMERICA to UK-WIDE audio;
- local listener talker-start, OPUS audio, flush and talker-stop notifications
  for an imported stream.

## Current limitations

- Only OPUS federation streams are supported.
- Peer and policy configuration is loaded at startup.
- Runtime hot reload and rollback are not implemented.
- Route advertisements and remote policy distribution are not implemented.
- There is no federation administration API or dashboard interface.
- Imported streams are not relayed to additional peers.
- X.509 certification and the reflector protocol 3.0 certificate
  infrastructure are implemented by the participating reflectors and remain
  available.
- Federation protocol and configuration compatibility are not yet declared
  stable.
