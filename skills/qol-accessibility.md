# Skill: QoL and Accessibility

Every important action must work with D-pad/buttons and touch. Touch selection should first highlight and require a deliberate second activation when a screen has nested choices. Keep focus/selection visible, use measured bitmap-font widths, and never depend on color alone.

English is the fallback language and Brazilian Portuguese is supported through `UI_TABLE`. Accessibility settings such as reduced screen shake must affect only presentation, not simulation. New strings need both locales or an English fallback. Scrollable lists need bounded indices and a visible control hint.
