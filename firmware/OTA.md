# Wireless firmware updates

ARCANA uses the existing public repository, `michaelDengl/arcana-mtg-life-counter`, as its firmware release source. It downloads a compiled application image from GitHub Releases. Source-code commits alone do not create an update.

## One-time USB setup

Earlier devices have one application partition and cannot install OTA images. Install the OTA-enabled firmware once using `tools/knob_device.py` and the verified backup for that exact device. Do not use `--initialize-storage` on an existing ARCANA. The installer validates the new partition map and initializes its OTA boot-selection data.

| Partition | Offset | Size |
| --- | --- | --- |
| NVS | `0x9000` | `0x6000` |
| PHY | `0xf000` | `0x1000` |
| OTA slot 0 | `0x10000` | `0x370000` |
| OTA slot 1 | `0x380000` | `0x370000` |
| OTA boot selection | `0x6f0000` | `0x2000` |
| Existing profile storage | `0x710000` | `0x8f0000` |

The persistent profile partition stays unchanged. The two application slots fit inside the old application region; each accommodates firmware up to 3,604,480 bytes. Wireless updates replace application code only, not the bootloader, partition table or profiles.

## Connect for updates

1. At startup, open **About > Wi-Fi & Updates > Wi-Fi setup**.
2. Scan the displayed Wi-Fi QR code with your phone and accept **Join**. It contains the temporary **ARCANA-SETUP-…** network name and password.
3. Follow the phone's Wi-Fi sign-in page or **Sign in to network** notification. Enter a 2.4 GHz home Wi-Fi network or compatible phone hotspot and save. If the page does not appear, open **http://192.168.4.1** in the phone's browser.
4. ARCANA stops its setup network, connects and checks the latest stable GitHub Release. Return the phone to its normal network.

Wi-Fi settings stay on the device in NVS. ARCANA stores no GitHub token. The setup access point is password protected, closes on Back, and expires after three minutes. The device checks once at startup when credentials exist and on manual Check; New Game cancels update networking so play retains its existing radio behavior.

The Wi-Fi QR joins the setup network; the captive portal handles opening the page. Phone confirmation and automatic-page behavior vary by operating system and settings. DNS advertises the setup AP as its local resolver and answers IPv4 queries with the knob's address; HTTP connectivity probes redirect to the setup form. HTTPS traffic is not intercepted. The DNS socket, HTTP server and setup-only DHCP options are removed when setup ends, including before multiplayer hosting. The QR disappears when its network is no longer active.

## Install an update

The startup **Update** button is disabled unless a newer compatible release has been found. Tap it, review the installed/latest versions, then tap **Install**. ARCANA displays progress, verifies the download and restarts. It uses the inactive slot so an interrupted download leaves the current application selected. A pending updated application must pass storage/display initialization and five seconds of main-loop operation to cancel the bootloader's rollback state.

Offline, connection failures, no release, or an incompatible release leave Update disabled. About > Wi-Fi & Updates provides the status, manual retry and Wi-Fi setup. OTA is unavailable during a live match.

## Prepare a release

Keep `src/core/AppInfo.hpp`, the top-level CMake project and `firmware/CMakeLists.txt` on the same numeric version. Build and test, then run:

```powershell
.\.tools\esp-python\Scripts\python.exe tools/package_firmware.py --tag v0.16.3
```

Packaging checks the built image's project, embedded version and size, then creates `build/release-assets/arcana-jc3636k718.bin`, a JSON description and `SHA256SUMS`. The binary filename is part of the model compatibility contract. GitHub supplies its asset digest; the device requires `sha256:` followed by 64 lowercase hex digits.

The included GitHub Actions workflow builds a version tag or an explicitly selected existing tag, uploads reviewable artifacts and prepares a **draft** release. Review the build and release before publishing. Devices ignore drafts and prereleases, and compare the latest stable release numerically against the installed version. Never put Wi-Fi passwords or GitHub credentials in the firmware/repository.

## Version 0.17.4

Cinder & Hex uses new portrait artwork. This release uses the existing application-only OTA layout.

## Version 0.17.3

After Dark and Cinder & Hex are available on page four of **Gear > Watch Face**. The two illustrated faces are included in the application image and use the existing application-only layout.

## Version 0.17.2

About shows **https://nova-dynamics.ch** instead of the personal email address. Its button opens a website QR code headed **NOVA DYNAMICS**. The MagicTG Kreuzlingen WhatsApp QR and Back navigation stay available.

This uses the existing application-only `arcana-ota-v1` update path. Earlier knobs with a single application partition need the one-time USB upgrade described above.

## Version 0.16.3

Adds **Mana Confluence**, the twelfth watch face, to **Gear > Watch Face**. Its bronze and ivory layout includes the five W/U/B/R/G mana glyphs, static stained-glass artwork, life tap/hold, rotary input, both counters, opponent tiles, Undo, battery and lethal warnings. The original eleven designs remain unchanged. The existing three-page selector and saved face/brightness settings continue to work.

This is a normal application-only OTA update for the existing `arcana-ota-v1` layout. No partition migration or USB reinstallation is required on an OTA-enabled knob. The USB installer also uses the newly built application image. Physical rendering performance of the new face still requires an owner-device check after installation.

## Version 0.16.2

This release includes all eleven approved watch faces on the physical knob. **Gear > Watch Face** opens a paged radial selector; the selected face and the chosen **Gear > Brightness** level survive restart. Older profiles migrate without losing the multiplayer name, stable identity or Wi-Fi setup. Match saving remains disabled.

The binary uses the existing `arcana-ota-v1` partition map and does not require another USB installation on a device already running OTA-enabled firmware. After installing, start a game and open the gear to choose a face. At saved 0% brightness, turn clockwise to recover a visible 10% without changing the match.

Native validation covers the selector and restart restoration in addition to life/hold, counters, Undo, lethal conditions, multiplayer names and layouts through twelve-player 2HG. The ESP32 image is built and size/version checked before packaging. The new faces still need the owner's physical-device check after the wireless installation; desktop tests do not measure physical rendering latency.

## Validation status

On 2026-11-22 the owner's device completed phone/browser provisioning and the HTTPS GitHub release check, then downloaded and installed v0.15.1 over Wi-Fi. USB monitoring confirmed the change from `ota_0` at `0x10000` to `ota_1` at `0x380000`, valid image state after boot, and a fresh release check reporting v0.15.1 as current. No USB application flashing was used for that update.

Native coverage checks startup button availability, About/contact and Wi-Fi QR navigation, QR payload escaping, bounded DNS parsing (including truncated/compressed/oversized packets and IPv6 probes), release version/model/size/digest/source validation, and rejection of partition maps that move profile storage. The v0.15.2 QR/captive-portal behavior still requires a physical phone check; it is prepared for the owner to install wirelessly later.
