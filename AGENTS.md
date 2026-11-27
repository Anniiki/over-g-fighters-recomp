# Project conventions

Write repository content in English. This covers documentation, code comments, diagnostics, commit messages, issue templates and release notes. Use ordinary hyphens or rewrite the sentence to avoid em dashes.

Keep game input outside the checkout. Keep generated translations and build output in ignored directories. Put persistent translation hints in the manifest or its included configuration, and put host-side fixes in handwritten source. Record the XEX fingerprint and guest address when documenting a title-specific fix.

Keep the public implementation in C++23. Build automation, launchers, benchmarks and test harnesses live in the local development workspace. The public manifest and translation hints describe the supported executable. Report campaign progress only after checking the game's own mission result and saved slot.
