# Mesa patches

Patches applied to upstream Mesa at a pinned commit before building the Zink and KosmicKrisp stack the macOS package ships.

Applied by `scripts/build-mesa.sh`, which owns the pin. Nothing here is applied by hand.

## Layout

| Path | Applied | Ships | Contents |
|---|---|---|---|
| `mesa/` | every build | yes | **empty** |
| `mesa-candidates/` | never automatically | no | 2 |
| `mesa-debug/` | only via `MESA_PATCH_DIR` | no | 1 |

Files are named `NNNN-slug.patch` and applied in `sort` order. The numbering is apply order and nothing else, so it encodes dependencies where they exist.

**`mesa/` is empty and that is the point.** We ship stock upstream Mesa at a pinned commit and carry no functional patches. Adding a file here changes that, and it should be a decision rather than a habit.

## Index

### `mesa/`, shipping

Nothing. See above.

### `mesa-candidates/`, staged but not applied

| Patch | Author | Applies to | Why it is staged |
|---|---|---|---|
| `0001-kk-One-MTL4CommandBuffer-per-VkCommandBuffer.patch` | Aitor Camacho | `73a03c61101` | Fixes [issue 15998](https://gitlab.freedesktop.org/mesa/mesa/-/work_items/15998). Our pin predates the bug, so it is only needed once the pin moves. Kept so the fix does not depend on an unmerged MR branch surviving on GitLab |
| `0002-zink-release-command-pool-resources-on-reset.patch` | ours | our pin, cleanly | Does nothing without `0001`. Worth 5 GB with it |

Apply order matters: `0002` is inert without `0001`.

### `mesa-debug/`, never ships

| Patch | Applies to | What it is |
|---|---|---|
| `0001-macOS-leak-diagnostics-zink-memory-and-blit-counters.patch` | `29b9c04a0da` | Counters for zink submits, blits and allocations, KosmicKrisp command buffers, and the VBO trace. The instrument for counting render passes |

## Apply status against the pin

Checked 2026-08-25 against `56588ef0665`:

| Patch | Result |
|---|---|
| `mesa-candidates/0002` | clean |
| `mesa-candidates/0001` | conflicts, expected, different base |
| `mesa-debug/0001` | conflicts, written ~1900 commits later, needs a rebase |

A conflict here is not rot. Each patch records its own `Applies-to` base and both were verified clean against theirs.

## What a patch must carry

Every patch has a metadata block between the subject line and the body:

```
Origin:      ours | <upstream MR url> | <repo> <path> @ <sha> <date>
Status:      shipping | debug-only | candidate
Applies-to:  mesa <pinned sha>
Expires:     when <specific condition>
Evidence:    <measurement, or "none, untested">
```

- **`Expires` is the important one.** A patch with no expiry condition is a fork rather than a patch. If you cannot write the condition, that is a decision to escalate, not a field to leave blank.
- **`Evidence` stops an untested patch riding along looking certified.** Write "none, untested" when that is the truth.

## Creating a patch

1. Branch from the pin in your Mesa clone
2. Commit the change with a real message
3. `git format-patch <pin>..<branch> -o <repo>/patches/mesa/`
4. Add the metadata block
5. Add a row to the index above

## Moving the pin

Because these are `git format-patch` files, the series is a branch:

```sh
git checkout -b rebase-<newpin> <newpin>
git am patches/mesa/*.patch          # fix conflicts as commits
git format-patch <newpin>..HEAD -o patches/mesa/
```

Update `Applies-to:` in each patch and the pin in `scripts/build-mesa.sh`.

## Taking a patch from another project

- **Never re-author.** If the file has a `From:` line, it stays untouched.
- If it has none, add `Origin:` naming repo, path, commit and date, and keep the original body verbatim, including any status block of theirs.
- Keep their slug. Do not renumber a foreign patch into a position that implies we wrote it.

`benbreen/RecoilEngine-AppleSilicon` carries a series at `patches/mesa/`. Note that those files have no author and no source commit, and there is no public Mesa fork behind them, so the finest attribution available is the repo, the path, and the commit that added the file.

Their pin is `8f272b1fe1`, which is 26.2-devel and post Metal 4. Ours is deliberately earlier. Anything taken from them needs rebasing onto our pin rather than applying as-is.

## Why the pin is where it is

`56588ef0665` is the commit before `c08dba83025 kk: Move to Metal4 command encoding`, which never returns the memory a render pass allocates. Anything after it leaks about 5 GiB a second under the engine. Filed as [issue 15998](https://gitlab.freedesktop.org/mesa/mesa/-/work_items/15998).

Full detail in `coding-agents/MACOS_BUILD.md` and `coding-agents/MACOS_PERFORMANCE.md`.
