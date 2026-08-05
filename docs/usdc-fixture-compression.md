# USDC fixture compression (clean-room)

FreeUSD does **not** implement Pixar production USDC compression. For tests and incremental parity, crate **section payloads** may use repo-owned wrappers documented here.

## Shared prefix

Each wrapper starts with a 7-byte magic and a little-endian `uint64_t` **uncompressed_size** (must match the decompressed byte length).

| Magic | Algorithm | Reference |
| --- | --- | --- |
| `FUSDZC\x01` | zlib (`deflate` / RFC 1950 wrapper via `uncompress`) | [RFC 1950](https://www.rfc-editor.org/rfc/rfc1950) |
| `FUSDZL\x01` | LZ4 block (no frame); decompressed by FreeUSD's bounded fixture block decoder | [LZ4 block format](https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md) |

Bytes after the 15-byte header are the compressed payload only.

## Fixtures

| File | Section | Wrapper |
| --- | --- | --- |
| `tests/fixtures/parity_tables.usdc` | `VALUES` | none (plain typed table) |
| `tests/fixtures/parity_tables_zlib.usdc` | `VALUES` | `FUSDZC` |
| `tests/fixtures/parity_tables_lz4.usdc` | `VALUES` | `FUSDZL` |
| `tests/fixtures/parity_embedded_scene_zlib.usdc` | `USDA` | `FUSDZC` |
| `tests/fixtures/parity_embedded_scene_lz4.usdc` | `USDA` | `FUSDZL` |

Regenerate with:

- `scripts/gen_parity_tables_usdc.py`
- `scripts/gen_parity_compressed_usdc.py`
- `scripts/gen_parity_lz4_usdc.py`
- `scripts/gen_parity_embedded_scene_zlib_usdc.py`
- `scripts/gen_parity_embedded_scene_lz4_usdc.py`

## Claims

- Validated only on these fixtures and the typed `VALUES` table shape.
- **Not** a claim about arbitrary production `.usdc` compression or TOC-level encoding.
