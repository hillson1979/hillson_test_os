#!/bin/bash
# Quick Task Selector - Shows next task and provides commands
#
# This script analyzes the feature list and shows what to work on next

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

cd "$(dirname "$0")/../.."

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Next Task Selector${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Get current progress
TOTAL=$(grep -c '"id":' harness/feature_list.json 2>/dev/null || echo "0")
PASSING=$(grep -c '"status": "passing"' harness/feature_list.json 2>/dev/null || echo "0")
if [ "$TOTAL" -gt 0 ]; then
    PERCENT=$((PASSING * 100 / TOTAL))
else
    PERCENT=0
fi

echo "Progress: $PASSING/$TOTAL features passing ($PERCENT%)"
echo ""

# Get next feature
NEXT_FEATURE=$(python3 <<'PYTHON'
import json

try:
    with open('harness/feature_list.json', 'r') as f:
        data = json.load(f)

    priority_order = ['critical', 'high', 'medium']

    for priority in priority_order:
        for cat_name, cat_data in data['categories'].items():
            if cat_data['priority'] != priority:
                continue
            for feature in cat_data['features']:
                if feature['status'] == 'failing':
                    result = {
                        'id': feature['id'],
                        'name': feature['name'],
                        'description': feature['description'],
                        'category': cat_data['name'],
                        'priority': cat_data['priority'],
                        'files': feature.get('files', [])
                    }
                    print(json.dumps(result))
                    exit(0)

    print("")

except Exception as e:
    print(f"Error: {e}")
    exit(1)
PYTHON
)

if [ -z "$NEXT_FEATURE" ]; then
    echo -e "${GREEN}✓${NC} All features passing!"
    echo ""
    echo "All 45 features are implemented and tested."
    echo "Great job!"
    exit 0
fi

# Parse feature
F_ID=$(echo "$NEXT_FEATURE" | python3 -c "import sys, json; d=json.load(sys.stdin); print(d['id'])")
F_NAME=$(echo "$NEXT_FEATURE" | python3 -c "import sys, json; d=json.load(sys.stdin); print(d['name'])")
F_DESC=$(echo "$NEXT_FEATURE" | python3 -c "import sys, json; d=json.load(sys.stdin); print(d['description'])")
F_CATEGORY=$(echo "$NEXT_FEATURE" | python3 -c "import sys, json; d=json.load(sys.stdin); print(d['category'])")
F_PRIORITY=$(echo "$NEXT_FEATURE" | python3 -c "import sys, json; d=json.load(sys.stdin); print(d['priority'])")
F_FILES=$(echo "$NEXT_FEATURE" | python3 -c "import sys, json; d=json.load(sys.stdin); print(','.join(d.get('files', [])))")

echo -e "${CYAN}Next Task:${NC} $F_ID"
echo -e "${CYAN}Name:${NC}     $F_NAME"
echo -e "${CYAN}Category:${NC} $F_CATEGORY"
echo -e "${CYAN}Priority:${NC} $F_PRIORITY"
echo -e "${CYAN}Description:${NC} $F_DESC"
echo ""
echo -e "${CYAN}Files:${NC}"
for file in ${F_FILES//,/ }; do
    if [ -f "$file" ]; then
        LINES=$(wc -l < "$file" 2>/dev/null || echo "0")
        echo "  ✓ $file ($LINES lines)"
    else
        echo "  ✗ $file (not found)"
    fi
done

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Commands to work on this task:${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "1. Edit files:"
for file in ${F_FILES//,/ }; do
    echo "   vim $file"
done
echo ""
echo "2. Build and test:"
echo "   wsl bash -c \"cd /mnt/f/hillson_test_os/zhwh_os && make clean && make && ./zh.sh\""
echo ""
echo "3. Test with QEMU:"
echo "   qemu-system-i386 -cdrom os.iso -vga std -display sdl -serial file:test_output.txt"
echo ""
echo "4. When done, mark as passing:"
echo "   ./harness/auto_workflow.sh --feature $F_ID"
echo ""
echo "Or use the automated workflow:"
echo "   ./harness/auto_workflow.sh"
echo ""
