#!/usr/bin/env python3
"""Regression tests for SVXReflector Federation Builder."""

import importlib.util
import sys
import unittest
from pathlib import Path
from unittest.mock import patch


BUILDER_PATH = (
    Path(__file__).resolve().parents[1]
    / "src/svxlink/reflector/svx_federation_builder.py"
)

spec = importlib.util.spec_from_file_location(
    "svx_federation_builder", BUILDER_PATH
)
builder = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = builder
spec.loader.exec_module(builder)


class TestMenuCancellation(unittest.TestCase):

    def test_choose_zero_cancels(self):
        with patch("builtins.input", return_value="0"):
            with self.assertRaises(builder.OperationCancelled):
                builder.choose("Test menu", ["First", "Second"])

    def test_choose_q_cancels(self):
        with patch("builtins.input", return_value="q"):
            with self.assertRaises(builder.OperationCancelled):
                builder.choose("Test menu", ["First", "Second"])

    def test_choose_valid_selection(self):
        with patch("builtins.input", return_value="2"):
            result = builder.choose("Test menu", ["First", "Second"])
        self.assertEqual(result, 1)

    def test_ask_q_cancels(self):
        with patch("builtins.input", return_value="q"):
            with self.assertRaises(builder.OperationCancelled):
                builder.ask("Test value")

    def test_ask_bool_q_cancels(self):
        with patch("builtins.input", return_value="q"):
            with self.assertRaises(builder.OperationCancelled):
                builder.ask_bool("Test confirmation")

    def test_ask_int_q_cancels(self):
        with patch("builtins.input", return_value="q"):
            with self.assertRaises(builder.OperationCancelled):
                builder.ask_int("Test number", minimum=1)

    def test_choose_many_zero_cancels(self):
        peers = [
            {"name": "NORTH-AMERICA"},
            {"name": "AUSTRALIA"},
        ]

        with patch("builtins.input", return_value="0"):
            with self.assertRaises(builder.OperationCancelled):
                builder.choose_many("Select peers", peers)


class TestPeerEditCancellation(unittest.TestCase):

    def test_cancel_partial_peer_edit_preserves_original(self):
        import copy

        plan = {
            "peers": [
                {
                    "name": "TEST-PEER",
                    "host": "original.example.net",
                    "reflector_id": "original.example.net",
                    "port": 35300,
                    "connect": True,
                    "trust_callsign": "REFLECTOR-TEST",
                }
            ],
            "talkgroups": [],
        }

        original = copy.deepcopy(plan)
        added = []
        removed = []

        # Change the hostname, then cancel at the port prompt.
        # Finally select 0 to return to the main menu.
        responses = iter([
            "3",
            "1",
            "changed.example.net",
            "q",
            "0",
        ])

        with patch("builtins.input", side_effect=responses):
            builder.edit_peers(plan, added, removed)

        self.assertEqual(plan, original)
        self.assertEqual(added, [])
        self.assertEqual(removed, [])


class TestTalkgroupEditCancellation(unittest.TestCase):

    def test_cancel_ownership_selection_preserves_talkgroup(self):
        import copy

        plan = {
            "reflector": {
                "domain": "TEST-DOMAIN",
            },
            "peers": [
                {"name": "TEST-PEER"}
            ],
            "talkgroups": [
                {
                    "tg": 999,
                    "description": "Original test talkgroup",
                    "ownership": "domain",
                    "home": "TEST-DOMAIN",
                    "distribution": "local",
                    "import_from": [],
                    "export_to": [],
                }
            ],
        }

        original = copy.deepcopy(plan)

        # Select ownership editing, choose TG 999,
        # select remote ownership, then cancel peer selection.
        # Finally return to the main menu.
        responses = iter([
            "5",
            "1",
            "2",
            "0",
            "0",
        ])

        with patch("builtins.input", side_effect=responses):
            builder.edit_talkgroups(plan)

        self.assertEqual(plan, original)


class TestMainMenuCancellation(unittest.TestCase):

    def test_main_menu_zero_exits_without_saving(self):
        import copy
        import tempfile

        plan = {
            "plan_schema": 1,
            "library_generation": 1,
            "reflector": {
                "domain": "TEST-DOMAIN",
                "reflector_id": "test.example.net",
                "callsign": "REFLECTOR-TEST",
                "library": "/etc/svxlink/federation.json",
                "enable": False,
            },
            "peers": [
                {
                    "name": "TEST-PEER",
                    "host": "peer.example.net",
                    "reflector_id": "peer.example.net",
                    "port": 35300,
                    "protocol": 2,
                    "connect": True,
                    "trust_callsign": "REFLECTOR-PEER",
                }
            ],
            "talkgroups": [],
        }

        original = copy.deepcopy(plan)

        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "test-plan.json"
            output = Path(temporary) / "generated"

            with (
                patch.object(builder, "load_plan", return_value=plan),
                patch.object(builder, "validate_plan", return_value=([], [])),
                patch.object(builder, "show_plan"),
                patch("builtins.input", return_value="0"),
            ):
                result = builder.edit_plan(path, output)

            self.assertEqual(result, 0)
            self.assertEqual(plan, original)
            self.assertFalse(path.exists())
            self.assertFalse(output.exists())

class TestFileIntegrity(unittest.TestCase):

    def test_cancelled_edit_preserves_existing_file(self):
        import tempfile

        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "test-plan.json"
            output = Path(temporary) / "generated"

            plan = {
                "plan_schema": 1,
                "library_generation": 1,
                "reflector": {
                    "domain": "TEST-DOMAIN",
                    "reflector_id": "test.example.net",
                    "callsign": "REFLECTOR-TEST",
                    "library": "/etc/svxlink/federation.json",
                    "enable": False,
                },
                "peers": [
                    {
                        "name": "TEST-PEER",
                        "host": "peer.example.net",
                        "reflector_id": "peer.example.net",
                        "port": 35300,
                        "protocol": 2,
                        "connect": True,
                        "trust_callsign": "REFLECTOR-PEER",
                    }
                ],
                "talkgroups": [],
            }

            builder.write_json(path, plan)
            original_bytes = path.read_bytes()

            # Change the proposed reflector domain, then cancel
            # the entire editing session without saving.
            responses = iter([
                "3",                 # Local reflector details
                "MODIFIED-DOMAIN",   # New domain
                "",                  # Keep hostname
                "",                  # Keep identity
                "",                  # Keep library path
                "",                  # Keep enable setting
                "0",                 # Exit without saving
            ])

            with patch("builtins.input", side_effect=responses):
                result = builder.edit_plan(path, output)

            self.assertEqual(result, 0)
            self.assertEqual(path.read_bytes(), original_bytes)
            self.assertFalse(output.exists())

            # No backup should have been created.
            self.assertEqual(
                list(Path(temporary).iterdir()),
                [path],
            )


if __name__ == "__main__":
    unittest.main()
