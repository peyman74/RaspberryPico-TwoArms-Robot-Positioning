# Quick Reference - PlatformIO Commands & Troubleshooting

## 🚀 Most Common Commands (Copy & Paste Ready!)

### Build Your Code
```powershell
pio run
```

### Upload to Pico
```powershell
pio run --target upload
```

### Open Serial Monitor
```powershell
pio device monitor
```

### Build + Upload (One Command)
```powershell
pio run --target upload
```

### Exit Serial Monitor
Press `Ctrl+C`

---

## 🔧 Troubleshooting - Common Problems & Solutions

### ❌ Problem: PlatformIO Frozen/Stuck (Build/Upload Not Working)
**Solution:** Kill stuck processes
```powershell
taskkill /F /IM pio.exe; taskkill /F /IM python.exe /FI "WINDOWTITLE eq *platformio*"
```
Then try your command again.

**Why this happens:** Usually after COM port changes or interrupted uploads.

---

### ❌ Problem: COM Port Changed
1. Open `platformio.ini`
2. Find these lines:
   ```ini
   upload_port = COM5
   monitor_port = COM5
   ```
3. Change COM5 to your new port number
4. Save file
5. **Close and reopen VS Code** (required for changes to take effect)

**How to find your COM port:**
```powershell
mode
```
Or check Device Manager → Ports (COM & LPT)

---

### ❌ Problem: Upload Failed - Pico Not Found
**Solution:** Put Pico in BOOTSEL mode manually:
1. Unplug Pico from USB
2. Hold BOOTSEL button on Pico
3. Plug USB cable back in while holding button
4. Release button - Pico appears as USB drive "RPI-RP2"
5. **Option A:** Run `pio run --target upload`
6. **Option B:** Manually copy `.pio\build\pico\firmware.uf2` to RPI-RP2 drive

---

### ❌ Problem: "Permission Denied" or "Access Denied" on COM Port
**Solution:** Close Serial Monitor first
```powershell
# Press Ctrl+C in the monitor window, then try upload again
```
**Why this happens:** Serial Monitor locks the COM port. You can't upload while monitoring.

---

### ❌ Problem: Want to See Detailed Error Messages
**Solution:** Add verbose flag
```powershell
pio run --target upload -v
```

---

### ❌ Problem: Code Compiles But Doesn't Work on Pico
**Solution:** Clean build and reupload
```powershell
pio run --target clean
pio run --target upload
```

---

## 🎯 VS Code Buttons vs Command Line

### ⚠️ RECOMMENDED: Use Command Line (More Reliable!)

The **toolbar buttons often freeze**, but **commands always work**. Just use these:
```powershell
pio run                    # Build
pio run --target upload    # Upload
pio device monitor         # Monitor
pio run --target clean     # Clean
```

### Toolbar Buttons (If They Work)
Look at the **bottom toolbar** in VS Code (PlatformIO icons):
- ✓ **Checkmark icon** = Build
- → **Arrow icon** = Upload  
- 🔌 **Plug icon** = Serial Monitor
- 🗑️ **Trash icon** = Clean

**Note:** If buttons freeze (common issue), just use the commands above instead.

---

## 📋 Other Useful Commands

### List All Available Commands
```powershell
pio run --list-targets
```

### Check PlatformIO Version
```powershell
pio --version
```

### Update PlatformIO
```powershell
pio upgrade
```

### Check Serial Devices
```powershell
pio device list
```

---

## 🆘 Emergency Full Reset

If **nothing** works and VS Code is completely frozen:

```powershell
# 1. Kill all PlatformIO processes
taskkill /F /IM pio.exe; taskkill /F /IM python.exe /FI "WINDOWTITLE eq *platformio*"

# 2. Close VS Code completely (File → Exit)

# 3. Reopen VS Code

# 4. Try build again
pio run
```

---

## 💡 Pro Tips

1. **Always build before upload** - It's automatic with `pio run --target upload`
2. **Check your COM port first** if upload fails - it changes sometimes
3. **Use BOOTSEL mode** as a fallback - it always works
4. **Close Serial Monitor** before uploading - can't do both at once
5. **Keep this file open** in a tab for quick reference!

---

## 📍 Current Configuration

Your Pico is configured on **COM5** in `platformio.ini`

To change it, edit:
```ini
upload_port = COM5
monitor_port = COM5
```
