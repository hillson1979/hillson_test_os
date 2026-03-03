#!/usr/bin/env python3
"""
Next Task Selector - Shows next task and provides commands
"""

import json
import sys
import os

def main():
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    CYAN = '\033[0;36m'
    NC = '\033[0m'

    # Change to project directory
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    os.chdir('..')

    print(f"{BLUE}========================================{NC}")
    print(f"{BLUE}Next Task Selector{NC}")
    print(f"{BLUE}========================================{NC}")
    print()

    # Load feature list
    try:
        with open('harness/feature_list.json', 'r') as f:
            data = json.load(f)
    except FileNotFoundError:
        print(f"Error: harness/feature_list.json not found")
        return 1
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in feature_list.json: {e}")
        return 1

    # Calculate progress
    total = data.get('total_features', 0)
    passing = data.get('completed_features', 0)

    if total > 0:
        percent = passing * 100 // total
    else:
        percent = 0

    print(f"Progress: {passing}/{total} features passing ({percent}%)")
    print()

    # Find next failing feature
    priority_order = ['critical', 'high', 'medium']
    next_feature = None

    for priority in priority_order:
        for cat_name, cat_data in data['categories'].items():
            if cat_data['priority'] != priority:
                continue
            for feature in cat_data['features']:
                if feature['status'] == 'failing':
                    next_feature = feature
                    next_feature['category'] = cat_data['name']
                    break
            if next_feature:
                break
        if next_feature:
            break

    if not next_feature:
        print(f"{GREEN}✓{NC} All features passing!")
        print()
        print("All 45 features are implemented and tested.")
        print("Great job!")
        return 0

    # Display feature info
    f_id = next_feature['id']
    f_name = next_feature['name']
    f_desc = next_feature['description']
    f_category = next_feature.get('category', 'Unknown')
    f_priority = next_feature.get('priority', 'Unknown')
    f_files = next_feature.get('files', [])

    print(f"{CYAN}Next Task:{NC} {f_id}")
    print(f"{CYAN}Name:{NC}     {f_name}")
    print(f"{CYAN}Category:{NC} {f_category}")
    print(f"{CYAN}Priority:{NC} {f_priority}")
    print(f"{CYAN}Description:{NC} {f_desc}")
    print()
    print(f"{CYAN}Files:{NC}")

    for file in f_files:
        if os.path.exists(file):
            with open(file, 'r') as f:
                lines = len(f.readlines())
            print(f"  ✓ {file} ({lines} lines)")
        else:
            print(f"  ✗ {file} (not found)")

    print()
    print(f"{BLUE}========================================{NC}")
    print(f"{BLUE}Commands to work on this task:{NC}")
    print(f"{BLUE}========================================{NC}")
    print()
    print("1. Edit files:")
    for file in f_files:
        print(f"   vim {file}")
    print()
    print("2. Build and test:")
    print("   wsl bash -c \"cd /mnt/f/hillson_test_os/zhwh_os && make clean && make && ./zh.sh\"")
    print()
    print("3. Test with QEMU:")
    print("   qemu-system-i386 -cdrom os.iso -vga std -display sdl -serial file:test_output.txt")
    print()
    print("4. When done, mark as passing:")
    print(f"   ./harness/update_progress.sh {f_id} passing")
    print()
    print("Or use the automated workflow:")
    print("   ./harness/auto_workflow.sh")
    print()

    return 0

if __name__ == '__main__':
    sys.exit(main())
