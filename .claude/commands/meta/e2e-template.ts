import { test } from '@playwright/test';

// Helper function to login as admin
async function loginAsAdmin(page) {
  await page.goto('http://localhost/rentemax');
  await page.waitForTimeout(1000);

  // Close cookie banner
  const cookieAccept = page.locator('[data-testid="cookie-consent-accept"]');
  if (await cookieAccept.isVisible({ timeout: 2000 }).catch(() => false)) {
    await cookieAccept.click();
  }

  // Login
  await page.locator('[data-testid="header-login-button"]').click();
  await page.waitForTimeout(500);
  await page
    .locator('[data-testid="login-email-input"] input')
    .fill('admin@sagdas.test');
  await page
    .locator('[data-testid="login-password-input"] input')
    .fill('SecurePassword123');
  await page.locator('[data-testid="login-submit-button"]').click();
  await page.waitForTimeout(2000);

  // Verify logged in by waiting for the user menu element
  const isLoggedIn = await page
    .locator('[data-testid="header-user-menu"]')
    .isVisible({ timeout: 5000 })
    .catch(() => false);
  if (!isLoggedIn) {
    throw new Error('Login failed');
  }
}

// E2E test to investigate a specific feature issue
test('investigate <feature> issue', async ({ page }) => {
  // 0. Login as admin
  await loginAsAdmin(page);

  // 1. Navigate
  await page.goto('<url>');
  await page.waitForTimeout(2000);

  // 2. Capture state
  await page.screenshot({ path: 'debug-<feature>.png', fullPage: true });
  const html = await page.content();
  console.log('HTML:', html.substring(0, 1000));

  // 3. Log console
  page.on('console', msg => console.log('BROWSER:', msg.text()));
  page.on('pageerror', err => console.log('ERROR:', err.message));

  // 4. Check elements
  const elementCount = await page.locator('<selector>').count();
  console.log('Elements found:', elementCount);

  // 5. Check loaded resources
  const scripts = await page.evaluate(() =>
    Array.from(document.querySelectorAll('script[src]')).map(s => s.src),
  );
  console.log('Scripts:', scripts);

  // 6. Check Vue app state
  const vueExists = await page.evaluate(
    () => typeof window.vueApp !== 'undefined',
  );
  console.log('Vue app loaded:', vueExists);
});
