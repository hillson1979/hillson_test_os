#!/bin/bash
# OS Development Harness - Session Initializer
# Based on: https://www.anthropic.com/engineering/effective-harnesses-for-long-running-agents
#
# This script sets up the development environment and provides current status overview.

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}zhwh_os Development Harness Init${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""

# Navigate to project root
cd "$(dirname "$0")/../.." || exit 1
PROJECT_ROOT=$(pwd)
OS_DIR="$PROJECT_ROOT/zhwh_os"

if [ ! -d "$OS_DIR" ]; then
    echo -e "${RED}Error: zhwh_os directory not found at $OS_DIR${NC}"
    exit 1
fi

echo -e "${GREEN}[1/7] Environment Check${NC}"
echo "Project root: $PROJECT_ROOT"
echo "OS directory: $OS_DIR"

# Check required tools
check_tool() {
    if command -v "$1" &> /dev/null; then
        echo -e "  ${GREEN}✓${NC} $1: $(command -v $1)"
    else
        echo -e "  ${YELLOW}⚠${NC} $1: NOT FOUND"
    fi
}

check_tool gcc
check_tool nasm
check_tool qemu-system-i386

echo ""
echo -e "${GREEN}[2/7] Git Status${NC}"
cd "$OS_DIR"

# Check current branch
CURRENT_BRANCH=$(git branch --show-current)
echo "  Current branch: ${BLUE}$CURRENT_BRANCH${NC}"

# Check for uncommitted changes
if [ -n "$(git status --porcelain)" ]; then
    echo -e "  ${YELLOW}⚠${NC} Uncommitted changes detected"
    git status --short | head -5
else
    echo -e "  ${GREEN}✓${NC} Working directory clean"
fi

echo ""
echo -e "${GREEN}[3/7] Build System${NC}"

# Generate offsets first
echo "  Generating task offsets..."
make gen-offsets > /dev/null 2>&1

# Build kernel
echo "  Building kernel..."
make clean > /dev/null 2>&1
make > build.log 2>&1

if [ -f kernel.bin ]; then
    echo -e "  ${GREEN}✓${NC} kernel.bin built successfully"
    KERNEL_SIZE=$(stat -c%s kernel.bin 2>/dev/null || stat -f%z kernel.bin)
    echo "  Size: $KERNEL_SIZE bytes"
else
    echo -e "  ${RED}✗${NC} kernel.bin build FAILED"
    echo "  Check build.log for details"
fi

echo ""
echo -e "${GREEN}[4/7] Progress Report${NC}"

# Show current progress
if [ -f harness/feature_list.json ]; then
    TOTAL=$(grep -c '"id":' harness/feature_list.json 2>/dev/null || echo "0")
    PASSING=$(grep -c '"status": "passing"' harness/feature_list.json 2>/dev/null || echo "0")
    FAILING=$(grep -c '"status": "failing"' harness/feature_list.json 2>/dev/null || echo "0")

    if [ "$TOTAL" -gt 0 ]; then
        PERCENT=$((PASSING * 100 / TOTAL))
        echo "  Total Features: $TOTAL"
        echo -e "  ${GREEN}Passing:${NC} $PASSING ($PERCENT%)"
        echo -e "  ${RED}Failing:${NC} $FAILING"
    else
        echo "  No features tracked yet"
    fi
else
    echo "  feature_list.json not found"
fi

echo ""
echo -e "${GREEN}[5/7] Next Priority Features${NC}"

# Show top 5 critical features that are failing
if [ -f harness/feature_list.json ]; then
    echo "  Critical priority features (failing):"
    grep -A 5 '"priority": "critical"' harness/feature_list.json | \
    grep -B 1 '"status": "failing"' | \
    grep '"id":' | head -3 | \
    sed 's/.*"\([^"]*\)".*/    - \1/' || echo "    None - all critical features passing!"
fi

echo ""
echo -e "${GREEN}[6/7] Session Initialization${NC}"

# Create new session
SESSION_ID=$(date +%Y%m%d_%H%M%S)
SESSION_DIR="harness/sessions/$SESSION_ID"

mkdir -p "$SESSION_DIR"

# Create session manifest
cat > "$SESSION_DIR/manifest.json" <<EOF
{
  "session_id": "$SESSION_ID",
  "start_time": "$(date -Iseconds 2>/dev/null || date)",
  "start_commit": "$(git log -1 --format=%H 2>/dev/null || echo 'unknown')",
  "branch": "$CURRENT_BRANCH",
  "status": "active",
  "features_worked": [],
  "tests_run": []
}
EOF

echo -e "  Session ID: ${BLUE}$SESSION_ID${NC}"
echo -e "  Directory: $SESSION_DIR"

# Start progress log
cat > "$SESSION_DIR/progress.log" <<EOF
# Session Progress Log
# Started: $(date)

## Initial State

EOF

echo -e "  ${GREEN}✓${NC} Session initialized"

echo ""
echo -e "${GREEN}[7/7] Quick System Test${NC}"

# Quick smoke test (timeout 5 seconds)
echo "  Running quick kernel boot test..."
timeout 5s qemu-system-i386 \
    -kernel kernel.bin \
    -m 512M \
    -display none \
    -serial file:"$SESSION_DIR/boot.log" \
    -no-reboot \
    2>/dev/null || true

if [ -f "$SESSION_DIR/boot.log" ]; then
    # Check if kernel booted
    if grep -q "task init" "$SESSION_DIR/boot.log" 2>/dev/null; then
        echo -e "  ${GREEN}✓${NC} Kernel boots successfully"
        BOOT_TIME=$(grep "task init" "$SESSION_DIR/boot.log" | head -1 | cut -d']' -f1)
        echo "  Boot time: $BOOT_TIME"
    else
        echo -e "  ${YELLOW}⚠${NC} Kernel boot unclear"
    fi
fi

echo ""
echo -e "${BLUE}======================================${NC}"
echo -e "${GREEN}Initialization Complete!${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""
echo -e "Next steps:"
echo -e "  1. Select a feature from feature_list.json"
echo -e "  2. Implement the feature"
echo -e "  3. Test thoroughly"
echo -e "  4. Update progress: ${YELLOW}./harness/update_progress.sh <FEATURE_ID> passing${NC}"
echo -e "  5. Commit changes"
echo ""
echo -e "Session files: $SESSION_DIR"
echo ""
