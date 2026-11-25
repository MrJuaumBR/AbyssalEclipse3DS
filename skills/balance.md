# Skill: Balance: Spells and Enemies

Balance is data-driven in `tools/data/mock_data.json` and regenerated into `source/data_gen.cpp`. Spells scale one rank at a time and use authored caps; the six orbit/summon spells remain capped at Rank 5 and Time-Stop at Rank 9. New effects must define trigger, cooldown, rank scaling, element, scope, and counterplay.

Enemies must have a readable role, bounded HP/damage/speed, a telegraph for dangerous attacks, and a season identity. Endless difficulty changes enemy pressure, not hidden instant kills. Any balance change needs a short rationale in `AI_Changes.md` and a catalog regeneration check.
