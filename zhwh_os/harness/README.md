# OS Development Harness

Based on: [Effective Harnesses for Long-Running Agents](https://www.anthropic.com/engineering/effective-harnesses-for-long-running-agents)

## Overview

This harness provides a structured approach to developing zhwh_os by tracking features, managing testing, and maintaining session history following the two-part agent pattern:

1. **Initializer Agent** (`init_session.sh`) - Sets up environment and provides status
2. **Coding Agents** - Make incremental progress with testing and commits

## Quick Start

### Starting a New Session

```bash
cd zhwh_os/harness
./init_session.sh
```

This will:
- Check environment (gcc, qemu, etc.)
- Build the kernel
- Show current progress
- Create a new session directory
- Run quick boot test
- Display next priority features

### Working on Features

1. **Select a feature** from `feature_list.json`
   ```bash
   cat feature_list.json | grep -A 10 '"id": "SI-001"'
   ```

2. **Implement the feature**
   - Read relevant source files
   - Make changes to code
   - Test locally

3. **Test the feature**
   ```bash
   ./test_feature.sh SI-001
   ```

4. **Update progress** (if tests pass)
   ```bash
   ./update_progress.sh SI-001 passing "Implement syscall entry/exit"
   ```

5. **Verify commit**
   ```bash
   git log -1 --oneline
   ```

## Harness Scripts

### `init_session.sh`
Initialize a new development session.

**Usage:**
```bash
./harness/init_session.sh
```

**What it does:**
- Checks environment (gcc, nasm, qemu)
- Shows git status
- Builds kernel with `make gen-offsets && make`
- Displays current progress
- Shows next priority features
- Creates session directory with timestamp
- Runs quick kernel boot test

### `test_feature.sh`
Test a specific feature.

**Usage:**
```bash
./harness/test_feature.sh <FEATURE_ID> [test_command]
```

**Examples:**
```bash
# Automated test
./harness/test_feature.sh SI-001

# Custom test
./harness/test_feature.sh SI-001 'qemu-system-i386 -kernel kernel.bin -serial file:test.log'
```

### `update_progress.sh`
Update feature progress and commit changes.

**Usage:**
```bash
./harness/update_progress.sh <FEATURE_ID> <passing|failing> [commit_message]
```

**Examples:**
```bash
./harness/update_progress.sh SI-001 passing "Implement syscall entry/exit"
./harness/update_progress.sh MM-002 failing "Need to fix page mapping"
```

## Feature List Format

Features are defined in `feature_list.json`:

```json
{
  "id": "SI-001",
  "name": "System Call Entry/Exit",
  "description": "Implement int $0x80 syscall mechanism",
  "status": "failing",
  "test_steps": [
    "Test syscall entry from user mode",
    "Verify syscall number is passed correctly",
    "Verify arguments are retrieved from trapframe",
    "Test return value is delivered correctly",
    "Verify errno is set on errors"
  ],
  "verification_command": "test_syscall_entry() in user program",
  "files": ["syscall.c", "vectors.s", "include/syscall.h"]
}
```

## Feature Status Values

- **`failing`**: Feature doesn't work or doesn't exist (initial state)
- **`passing`**: All tests pass, ready to commit

## Workflow Example

```bash
# 1. Start new session
cd zhwh_os/harness
./init_session.sh

# Output shows:
#   Current branch: dev-harness
#   Passing: 4/45 (9%)
#   Next priority: SI-001, TM-003, IH-005

# 2. Select feature SI-001 (System Call Entry/Exit)
cd ../..
vim syscall.c  # Make changes
make           # Build

# 3. Test the feature
cd harness
./test_feature.sh SI-001

# Output: ✓ Feature SI-001: PASSED

# 4. Update progress and commit
./update_progress.sh SI-001 passing "Implement syscall entry/exit"

# Output: Changes committed with detailed message

# 5. Continue to next feature
./init_session.sh  # Check updated status
```

## Session Structure

```
harness/
├── feature_list.json      # Complete feature catalog
├── init_session.sh        # Session initializer
├── test_feature.sh        # Feature tester
├── update_progress.sh     # Progress updater
├── sessions/              # Historical session data
│   └── 20260303_120000/
│       ├── manifest.json  # Session metadata
│       ├── progress.log   # Session log
│       ├── boot.log       # Test outputs
│       └── test_*.log     # Feature test logs
└── README.md             # This file
```

## Important Rules

1. **Never mark features as passing without testing**
2. **Always commit before marking features as passing**
3. **Test edge cases, not just happy paths**
4. **Keep commits small and focused**
5. **One feature per commit**

## Progress Tracking

Current progress is tracked in `feature_list.json`:

```bash
# Check current status
cat harness/feature_list.json | grep -A 2 '"total_features"'

# Output:
#   "total_features": 45,
#   "completed_features": 4,
```

## Git Workflow

The harness follows a simple git workflow:

1. Development happens on feature branches (e.g., `dev-harness`)
2. Each completed feature gets a commit
3. Merge to master when satisfied

```bash
# View commits since master
git log master..dev-harness --oneline

# Merge to master (when ready)
git checkout master
git merge dev-harness
git push origin master
```

## Testing

### Quick Test
```bash
cd zhwh_os
make
qemu-system-i386 -kernel kernel.bin -m 512M -serial file:test.log
```

### Feature Test
```bash
cd harness
./test_feature.sh <FEATURE_ID>
```

### Full Test with User Program
```bash
cd zhwh_os
./zh.sh
qemu-system-i386 -cdrom os.iso -vga std -display sdl
```

## Troubleshooting

### Test fails but feature works
- Check test output in session log
- Verify test steps match actual usage
- Update verification command if needed

### Build fails
- Check `make` output
- Verify dependencies are installed
- Check `gen-offsets` ran successfully

### Kernel won't boot
- Check serial output log
- Verify trapframe offsets match
- Test with minimal configuration

## References

- [TRAPFRAME_LAYOUT_FIX.md](../TRAPFRAME_LAYOUT_FIX.md) - Trapframe bug documentation
- [OFFSET_AUTO_GENERATION.md](../OFFSET_AUTO_GENERATION.md) - Offset generation
- [Feature List](feature_list.json) - Complete feature catalog
