# Skill: Save System

`SaveData` is versioned and CRC-checked. Writes use alternating generation-numbered copies plus a v6 recovery copy; invalid data must never be silently replaced by defaults. New fields require a version bump, explicit migration, safe zero/default values, and preservation of legacy source files.

Persistent changes are written on meaningful events rather than every frame. Coin balance is a saturating 32-bit value split into `coinsLo` and `coinsHi`. Any new shop purchase must be atomic with the balance update and must remain safe when a recovery write is blocked. Test fresh, migrated, corrupt, and interrupted-write paths.
