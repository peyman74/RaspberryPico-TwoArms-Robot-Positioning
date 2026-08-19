# Git Commands Cheat Sheet (DcServoControl)

All commands assume the terminal is opened in this folder:

```bash
cd "C:\Users\Peyman\OneDrive\Summer2025-SSTM\ProjectA\PlatformIO\DcServoControl"
```

---

## 1. See what changed

```bash
git status           # Show changed/untracked files
git diff             # Show detailed changes in tracked files
```

See changes for a single file:

```bash
git diff src/main.cpp
```

---

## 2. Save a snapshot (commit)

After you are happy with the changes and the code builds:

```bash
git add .                            # Stage all changes
git commit -m "Describe the change"  # Create a snapshot
```

Examples:

```bash
git commit -m "Tune PID parameters for Motor 1"
git commit -m "Fix watchdog timing bug"
```

---

## 3. Undo changes (go back to last commit)

Discard **all** uncommitted changes and restore the last committed state:

```bash
git reset --hard HEAD
```

Discard changes for a **single** file:

```bash
git restore src/main.cpp
# or older syntax
git checkout -- src/main.cpp
```

If you already staged something with `git add` but want to unstage (keep file changes):

```bash
git reset HEAD src/main.cpp
```

---

## 4. See history

```bash
git log                 # Full history
git log --oneline       # Short one-line history
```

Show who changed a specific file recently:

```bash
git log --oneline src/main.cpp
```

---

## 5. Create and switch branches (optional)

Create a new branch for experiments:

```bash
git branch test-tuning          # Create branch
git switch test-tuning          # Switch to it
# or
git checkout -b test-tuning
```

Go back to main branch:

```bash
git switch main
# or
git checkout main
```

Delete an experiment branch after you are done (and merged or no longer need it):

```bash
git branch -d test-tuning
```

---

## 6. Connect to GitHub (backup)

1. Create an **empty private repository** on GitHub (no README, no .gitignore).
2. In this project folder, add the remote (replace URL with your repo URL):

```bash
git remote add origin https://github.com/<your-user>/DcServoControl.git
git branch -M main
```

3. Push the current snapshot to GitHub:

```bash
git push -u origin main
```

Next time you want to update the backup after new commits:

```bash
git push
```

---

## 7. Typical daily workflow

1. Open project and code.
2. Build & test with PlatformIO.
3. When everything works:
   - `git status` (check what changed)
   - `git diff` (optional, review changes)
   - `git add .`
   - `git commit -m "Short description"`
   - `git push` (if remote is configured)
4. If something goes badly wrong before committing:
   - `git reset --hard HEAD`  (restores last good snapshot).

---

## 8. VS Code integration (no terminal)

- Use the **Source Control** tab (Git icon on the left):
  - See all changed files.
  - Click a file to see a side-by-side diff.
  - Right-click a file → **Discard Changes** to undo edits back to last commit.
  - Enter a message at the top and click the checkmark icon to commit.

This file is just a quick reference. When in doubt, you can always run:

```bash
git status
```

to see the current state and next hints from Git.
