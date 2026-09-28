# Releasing

How to build a signed, notarized installer and publish it.

## One-time setup

1. **Certificates.** In Xcode, go to Settings > Accounts > your team > Manage Certificates, and add a **Developer ID Application** and a **Developer ID Installer** certificate.
2. **App-specific password.** At account.apple.com > Sign-In and Security > App-Specific Passwords, create one for notarization.
3. **Save the notary login:**
   ```
   xcrun notarytool store-credentials "cvfuzz-notary" --apple-id YOUR_APPLE_ID --team-id YOUR_TEAM_ID --password APP_SPECIFIC_PASSWORD
   ```

## Build a release

1. Set the version at the top of `CMakeLists.txt`, for example `project(CVFuzz VERSION 1.0.1)`.
2. Run:
   ```
   export DEV_ID_APP="Developer ID Application: Your Name (TEAMID)"
   export DEV_ID_INSTALLER="Developer ID Installer: Your Name (TEAMID)"
   export NOTARY_PROFILE="cvfuzz-notary"
   ./build-mac.sh --installer
   ```
   This builds, signs, uploads to Apple, waits for approval, and staples the result.
3. Confirm the output ends with `accepted` and `source=Notarized Developer ID`. The installer is in `dist/`.

## Publish on GitHub

1. On the repository page, open **Releases** > **Draft a new release**.
2. Create a tag matching the version, like `v1.0.1`.
3. Attach `dist/CV-Fuzz-macOS.pkg` (the script makes this copy with a fixed name) and publish.
4. Publishing starts the **Release builds** workflow. About 15 minutes later it attaches `CV-Fuzz-Windows-Setup.exe` and `CV-Fuzz-Linux-x64.tar.gz` to the same release.

The website's download buttons use GitHub's `releases/latest/download/<file>` links, so they always point at the newest release as long as the file names stay the same.

## If notarization fails

Get Apple's detailed log:

```
xcrun notarytool log <submission-id> --keychain-profile "cvfuzz-notary"
```

It names the exact file Apple objected to.
