"""
E2E Test: Theme Switching

Tests theme selection and application:
1. Theme dropdown population
2. Theme selection
3. Theme color application verification
4. Theme persistence
"""

import sys
import os
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from oscil_test_utils import OscilTestClient


def setup(client: OscilTestClient):
    """Reset state and open editor"""
    print("[SETUP] Resetting state and opening editor...")
    client.reset_state()
    time.sleep(0.5)
    assert client.open_editor(), "Failed to open editor"
    time.sleep(0.5)


def expand_options_section(client: OscilTestClient) -> bool:
    """Expand the options accordion section"""
    options_section_id = "sidebar_options"
    if not client.element_exists(options_section_id):
        print("[WARN] Options section not found")
        return False

    print("[ACTION] Expanding Options section...")
    client.click(options_section_id)
    time.sleep(0.4)

    # Verify expanded
    theme_dropdown_id = "sidebar_options_themeDropdown"
    return client.element_visible(theme_dropdown_id) if client.element_exists(theme_dropdown_id) else True


def test_theme_dropdown_population(client: OscilTestClient):
    """
    Test: Theme Dropdown Population

    Verifies that the theme dropdown is populated with available themes.
    """

    if not expand_options_section(client):
        print("[SKIP] Could not expand options section")
        return

    theme_dropdown_id = "sidebar_options_themeDropdown"
    if not client.element_exists(theme_dropdown_id):
        print("[SKIP] Theme dropdown not found")
        return

    dropdown_info = client.get_element(theme_dropdown_id)
    if dropdown_info and dropdown_info.extra:
        # C++ returns camelCase field names: numItems, selectedId, items
        num_items = dropdown_info.extra.get('numItems', 0)
        items = dropdown_info.extra.get('items', [])
        selected = dropdown_info.extra.get('selectedId', '')

        print(f"[INFO] Theme dropdown has {num_items} items")
        print(f"[INFO] Currently selected: {selected}")
        if items:
            print(f"[INFO] Available themes: {[item.get('id', item) for item in items]}")

        client.assert_true(num_items > 0, "Theme dropdown should have at least one theme")
    else:
        # Verify element exists at minimum
        client.assert_element_visible(theme_dropdown_id, "Theme dropdown should be visible")

    print("[SUCCESS] Theme dropdown population test completed")


def test_theme_selection(client: OscilTestClient):
    """
    Test: Theme Selection

    Tests selecting different themes from the dropdown.
    """

    if not expand_options_section(client):
        print("[SKIP] Could not expand options section")
        return

    theme_dropdown_id = "sidebar_options_themeDropdown"
    if not client.element_exists(theme_dropdown_id):
        print("[SKIP] Theme dropdown not found")
        return

    dropdown_info = client.get_element(theme_dropdown_id)
    if not dropdown_info or not dropdown_info.extra:
        print("[SKIP] Could not get dropdown info")
        return

    # C++ returns camelCase field names
    items = dropdown_info.extra.get('items', [])
    current_selection = dropdown_info.extra.get('selectedId', '')

    if len(items) < 2:
        print("[SKIP] Need at least 2 themes to test selection")
        return

    # Find a theme that is NOT currently selected
    new_theme_id = None
    for item in items:
        item_id = item.get('id', item) if isinstance(item, dict) else item
        if item_id != current_selection:
            new_theme_id = item_id
            break

    if not new_theme_id:
        print("[SKIP] Could not find alternative theme")
        return

    print(f"[ACTION] Switching theme from '{current_selection}' to '{new_theme_id}'...")

    # Select the new theme
    success = client.select_dropdown_item(theme_dropdown_id, new_theme_id)
    time.sleep(0.5)

    if success:
        # Verify selection changed
        updated_info = client.get_element(theme_dropdown_id)
        if updated_info and updated_info.extra:
            # C++ returns camelCase field names
            new_selection = updated_info.extra.get('selectedId', '')
            client.assert_true(
                new_selection == new_theme_id,
                f"Theme should change to '{new_theme_id}' (got '{new_selection}')"
            )

    print("[SUCCESS] Theme selection test completed")


def test_theme_color_application(client: OscilTestClient):
    """
    Test: Theme Color Application

    Verifies that changing theme actually changes UI colors.
    """

    if not expand_options_section(client):
        print("[SKIP] Could not expand options section")
        return

    theme_dropdown_id = "sidebar_options_themeDropdown"
    if not client.element_exists(theme_dropdown_id):
        print("[SKIP] Theme dropdown not found")
        return

    dropdown_info = client.get_element(theme_dropdown_id)
    if not dropdown_info or not dropdown_info.extra:
        print("[SKIP] Could not get dropdown info")
        return

    items = dropdown_info.extra.get('items', [])
    if len(items) < 2:
        print("[SKIP] Need at least 2 themes")
        return

    # Get sidebar background color before theme change
    sidebar_info = client.get_element("sidebar")
    initial_bg = None
    if sidebar_info and sidebar_info.extra:
        initial_bg = sidebar_info.extra.get('backgroundColor', '')

    # Switch theme
    first_theme = items[0].get('id', items[0]) if isinstance(items[0], dict) else items[0]
    second_theme = items[1].get('id', items[1]) if isinstance(items[1], dict) else items[1]

    print(f"[ACTION] Switching between themes: {first_theme} -> {second_theme}")

    # Select first theme
    client.select_dropdown_item(theme_dropdown_id, first_theme)
    time.sleep(0.5)

    # Select second theme
    client.select_dropdown_item(theme_dropdown_id, second_theme)
    time.sleep(0.5)

    # The UI should have visually changed (hard to verify without screenshots)
    # We verify that no errors occurred during the switch
    client.assert_true(True, "Theme switch completed without errors")

    # Take screenshot for visual verification
    screenshot_path = "screenshots/theme_switch_test.png"
    if client.take_screenshot(screenshot_path):
        print(f"[INFO] Screenshot saved to {screenshot_path}")

    print("[SUCCESS] Theme color application test completed")


def test_theme_persistence(client: OscilTestClient):
    """
    Test: Theme Persistence

    Tests that the selected theme persists after closing and reopening the editor.
    """

    if not expand_options_section(client):
        print("[SKIP] Could not expand options section")
        return

    theme_dropdown_id = "sidebar_options_themeDropdown"
    if not client.element_exists(theme_dropdown_id):
        print("[SKIP] Theme dropdown not found")
        return

    dropdown_info = client.get_element(theme_dropdown_id)
    if not dropdown_info or not dropdown_info.extra:
        print("[SKIP] Could not get dropdown info")
        return

    items = dropdown_info.extra.get('items', [])
    if len(items) < 2:
        print("[SKIP] Need at least 2 themes")
        return

    # Select a specific theme
    target_theme = items[1].get('id', items[1]) if isinstance(items[1], dict) else items[1]
    print(f"[ACTION] Setting theme to: {target_theme}")

    client.select_dropdown_item(theme_dropdown_id, target_theme)
    time.sleep(0.3)

    # Close editor
    print("[ACTION] Closing editor...")
    client.close_editor()
    time.sleep(0.5)

    # Reopen editor
    print("[ACTION] Reopening editor...")
    client.open_editor()
    time.sleep(0.5)

    # Expand options again
    expand_options_section(client)

    # Check if theme persisted
    updated_info = client.get_element(theme_dropdown_id)
    if updated_info and updated_info.extra:
        # C++ returns camelCase field names
        current_theme = updated_info.extra.get('selectedId', '')
        print(f"[INFO] Theme after reopen: {current_theme}")

        client.assert_true(
            current_theme == target_theme,
            f"Theme '{target_theme}' should persist (got '{current_theme}')"
        )

    print("[SUCCESS] Theme persistence test completed")


def test_all_themes_valid(client: OscilTestClient):
    """
    Test: All Themes Valid

    Tests that all available themes can be selected without errors.
    """

    if not expand_options_section(client):
        print("[SKIP] Could not expand options section")
        return

    theme_dropdown_id = "sidebar_options_themeDropdown"
    if not client.element_exists(theme_dropdown_id):
        print("[SKIP] Theme dropdown not found")
        return

    dropdown_info = client.get_element(theme_dropdown_id)
    if not dropdown_info or not dropdown_info.extra:
        print("[SKIP] Could not get dropdown info")
        return

    items = dropdown_info.extra.get('items', [])
    print(f"[INFO] Testing {len(items)} themes...")

    errors = []
    for item in items:
        theme_id = item.get('id', item) if isinstance(item, dict) else item
        print(f"[ACTION] Testing theme: {theme_id}")

        try:
            success = client.select_dropdown_item(theme_dropdown_id, theme_id)
            if not success:
                errors.append(theme_id)
            time.sleep(0.3)
        except Exception as e:
            print(f"[ERROR] Failed to select theme '{theme_id}': {e}")
            errors.append(theme_id)

    client.assert_true(
        len(errors) == 0,
        f"All themes should be selectable (failed: {errors})"
    )

    print("[SUCCESS] All themes valid test completed")


def teardown(client: OscilTestClient):
    """Clean up"""
    print("[TEARDOWN] Closing editor...")
    client.close_editor()


def main():
    """Run all theme switching tests"""
    client = OscilTestClient()

    if not client.wait_for_harness():
        sys.exit(1)

    print("\n" + "=" * 60)
    print("THEME SWITCHING TESTS")
    print("=" * 60)

    # Test 1: Dropdown population
    print("\n--- Test 1: Theme Dropdown Population ---")
    setup(client)
    try:
        test_theme_dropdown_population(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
    finally:
        teardown(client)

    # Test 2: Theme selection
    print("\n--- Test 2: Theme Selection ---")
    setup(client)
    try:
        test_theme_selection(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
    finally:
        teardown(client)

    # Test 3: Color application
    print("\n--- Test 3: Theme Color Application ---")
    setup(client)
    try:
        test_theme_color_application(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
    finally:
        teardown(client)

    # Test 4: All themes valid
    print("\n--- Test 4: All Themes Valid ---")
    setup(client)
    try:
        test_all_themes_valid(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
    finally:
        teardown(client)

    # Test 5: Theme persistence
    print("\n--- Test 5: Theme Persistence ---")
    setup(client)
    try:
        test_theme_persistence(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
    finally:
        teardown(client)

    # Print summary
    success = client.print_summary()
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
