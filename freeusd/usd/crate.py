"""USD container sniffing plus low-level USDC bootstrap, TOC, raw section bytes, and validated table decode."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usd.crate

UsdFileKind = _m.UsdFileKind
detect_usd_file_kind_from_path = _m.detect_usd_file_kind_from_path
read_usdc_bootstrap_from_path = _m.read_usdc_bootstrap_from_path
read_usdc_fields_table_from_path = _m.read_usdc_fields_table_from_path
read_usdc_specs_table_from_path = _m.read_usdc_specs_table_from_path
read_usdc_fieldsets_table_from_path = _m.read_usdc_fieldsets_table_from_path
read_usdc_path_table_from_path = _m.read_usdc_path_table_from_path
read_usdc_section_bytes_from_path = _m.read_usdc_section_bytes_from_path
read_usdc_string_table_from_path = _m.read_usdc_string_table_from_path
read_usdc_token_table_from_path = _m.read_usdc_token_table_from_path
read_usdc_toc_from_path = _m.read_usdc_toc_from_path
usdc_crate_identifier = _m.usdc_crate_identifier

__all__ = [
    "UsdFileKind",
    "detect_usd_file_kind_from_path",
    "read_usdc_bootstrap_from_path",
    "read_usdc_fields_table_from_path",
    "read_usdc_specs_table_from_path",
    "read_usdc_fieldsets_table_from_path",
    "read_usdc_path_table_from_path",
    "read_usdc_section_bytes_from_path",
    "read_usdc_string_table_from_path",
    "read_usdc_token_table_from_path",
    "read_usdc_toc_from_path",
    "usdc_crate_identifier",
]
