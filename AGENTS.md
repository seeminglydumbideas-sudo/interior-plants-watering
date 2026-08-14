# Open Source Pre-Commit Checklist

Before pushing any commits to this repository, AI agents and developers must verify the following privacy and security rules:

## 1. Privacy & Personal Data
* **Scrub Metadata from Media:** Ensure EXIF and other metadata are completely stripped from all photos and media files (e.g., using `exiftool -all= <file>`) before committing.
* **No Absolute Paths:** Ensure all file paths are relative to the project root. Absolute paths that reveal local system usernames (e.g., `/home/user/...`) must be removed.
* **No Personal Information:** Ensure no files contain personally identifiable information (PII) such as full names, personal email addresses, or exact physical locations.
* **No Sensitive Network Data:** Ensure no private IP addresses, MAC addresses, or internal network hostnames are hardcoded in the source code or configuration files.

## 2. Secrets & Credentials
* **No Hardcoded Secrets:** Ensure no API keys, Wi-Fi SSIDs, passwords, or authentication tokens are ever committed. Keep these in `.gitignore`d files like `Secrets.cpp`.
* **Gitignore Validation:** Verify that files containing local environment details (like IDE configurations or local build caches) are correctly listed in `.gitignore`.

## 3. AI & Text Quality Guidelines
* **Documentation Refinement:** AI agents are encouraged to review and reformulate plain-text documentation into well-written English at any time.
* **Argument Validation & Sourcing:** Agents must verify technical arguments and factual claims. If a user provides flawed arguments, the agent should warn them. All references must be verified online, and the agent should maintain a separate reference document containing the URL, a short summary, context for where the reference applies, and the date it was last checked.
* **Decision Logging:** Agents should maintain a decision log in a separate file to track architectural, design, and implementation choices over time.