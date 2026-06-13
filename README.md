# Forcivate_Interview_Task_Habiba_Elsayed
Technical Assessment for the Full Stack Internship Position
# Forcivate Comment-Response Engine
A C++/Python hybrid service that ingests social media posts and comments, drafts
replies using a local LLM, runs them through an automated safety gate, and routes
them through a human approval CLI before mock-publishing.

---

## Project Structure

```
Forcivate Project/
├── cpp/
│   ├── main.cpp
│   ├── CMakeLists.txt
│   ├── services/
│   │   ├── DraftEngine.cpp / .h      # pybind11 bridge to Python LLM layer
│   │   ├── SafetyGate.cpp / .h       # injection detection + sanitization
│   │   └── ReviewQueue.cpp / .h      # interactive CLI reviewer
│   ├── config/
│   │   └── VoiceConfig.cpp / .h      # platform-aware brand voice config
│   └── python/
│       ├── llm_adapter.py            # local GGUF model wrapper
│       └── models/
│           └── model.gguf            # local LLM model file (not in repo)
├── data/
│   └── comments.json                 # input fixture
├── output/                           # generated at runtime
│   ├── drafts.json
│   ├── approvals.json
│   └── published.json
└── json.hpp                          # nlohmann/json single-header library
```

---

## Prerequisites

| Requirement | Version |
|---|---|
| CMake | 3.16+ |
| C++ compiler | C++17 (MSVC, GCC, or Clang) |
| Python | 3.9+ |
| pybind11 | Any recent version |
| llama-cpp-python | Any recent version |
| A GGUF model file | e.g. Mistral, Llama 3, Phi-3 |

Install Python dependencies:

```bash
pip install pybind11 llama-cpp-python
```

Place your GGUF model file at:

```
cpp/python/models/model.gguf
```

---

## Building

```bash
cd cpp
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

The executable is output to `cpp/build/Release/forcivate.exe` on Windows or
`cpp/build/forcivate` on Linux/macOS.

---

## Running

From the project root:

```bash
# Windows
.\cpp\build\Release\forcivate.exe

# Linux / macOS
./cpp/build/forcivate
```

Make sure the `output/` directory exists before running:

```bash
mkdir output
```

---

## Pipeline

Each run executes the following steps in order:

```
comments.json
      │
      ▼
  [1] Ingest
      Parse all posts and comments from data/comments.json.
      │
      ▼
  [2] Safety Gate  (C++ — SafetyGate)
      Every comment is evaluated before anything else.
      │
      ├─ FLAGGED (spam / prompt injection) → blocked, logged, skipped
      ├─ HIGH_RISK (hostile language)      → canned response, goes to review
      └─ SAFE                              → sanitized, passed to LLM
      │
      ▼
  [3] Sanitize  (C++ — SafetyGate::sanitize)
      Injection keywords and prompt-structure labels are stripped from the
      comment text BEFORE it reaches the Python layer.
      │
      ▼
  [4] Draft Generation  (Python — llm_adapter.py via pybind11)
      The sanitized comment and post are passed to the local GGUF model.
      Brand voice (tone, emoji, length) is sourced from VoiceConfig.
      Few-shot examples from learning_examples.json guide style.
      │
      ▼
  [5] Human Review CLI  (C++ — ReviewQueue)
      Reviewer sees: comment, draft reply, risk level.
      Options: (A)pprove  (E)dit  (R)eject
      │
      ├─ Approved / Edited → published.json + learning_examples.json
      └─ Rejected          → logged to approvals.json only
      │
      ▼
  [6] Mock Publish
      Approved replies are written to output/published.json.
      No real API calls are made.
```

---

## Output Files

All files are written to `output/` at the end of each run.

**`drafts.json`** — every generated draft before review:
```json
[
  { "comment_id": "c1_genuine", "draft": "Great question! Yes, we support native Slack..." }
]
```

**`approvals.json`** — every review decision:
```json
[
  { "comment_id": "c1_spam",    "decision": "flagged",  "reason": "Safety Gate" },
  { "comment_id": "c1_genuine", "decision": "approved", "reply": "..." }
]
```

**`published.json`** — mock-published replies only:
```json
[
  { "comment_id": "c1_genuine", "published_reply": "..." }
]
```

---

## Safety Gate

All comment text is treated as **untrusted external input**. The gate runs two passes:

**Pass 1 — `evaluate()`** classifies the comment and decides whether to block it.

| Flag | Trigger | Action |
|---|---|---|
| `FLAGGED` | Spam patterns or prompt-injection keywords | Blocked — never reaches LLM |
| `HIGH_RISK` | Hostile / abusive language | Canned response, goes to human review |
| `SAFE` | None of the above | Drafted by LLM normally |

**Pass 2 — `sanitize()`** strips injection directive language from the text using
case-insensitive regex before it is forwarded to Python. This is a defence-in-depth
measure: even a comment that scores `SAFE` may contain partial injection phrases.

Phrases removed include: `ignore all previous instructions`, `respond only with`,
`output only`, `Reply:`, `Post:`, `Comment:` and others.

To add a new blocked phrase, add a pattern to the `injectionPatterns` vector in
`SafetyGate.cpp` — no other file needs to change.

---

## Brand Voice

`VoiceConfig` maps a platform name to tone parameters:

| Platform | Tone | Emoji | Length |
|---|---|---|---|
| `shortform` | casual | yes | short |
| `professional` | professional | no | medium |
| *(other)* | neutral | no | medium |

To change the voice for a platform, edit `VoiceConfig.cpp`.

---

## Learning Signal

When a reviewer approves or edits a reply, it is appended to
`cpp/python/learning_examples.json` and used as a few-shot style example on the
next run (last 3 examples are injected into the prompt).

> **Important:** if the model starts generating malformed replies, clear
> `learning_examples.json` back to `[]`. Corrupted entries from bad generations
> will poison subsequent prompts.

---

## Design Decisions

**Why C++ for the core and Python for the LLM?**
The safety gate and review workflow are deterministic control-flow logic — C++ is
the right tool and keeps the trust boundary explicit. The LLM layer changes
frequently and is easier to iterate on in Python. pybind11 bridges them with no
IPC overhead.

**Why a local GGUF model?**
The assignment requires the project to run on a standard laptop without cloud
infrastructure. A local GGUF model via `llama-cpp-python` satisfies this with no
API key required.

**Why is `sanitize()` separate from `evaluate()`?**
`evaluate()` classifies — it answers "is this dangerous enough to block?"
`sanitize()` hardens — it answers "can we strip anything that could manipulate
the model?" Keeping them separate makes each responsibility testable independently.

---

## Known Limitations & What I Would Improve Next

- **Keyword-based safety gate** — a small classifier model would catch paraphrased
  injections that keyword lists miss.
- **Canned hostile response** — hostile comments get a hardcoded reply rather than
  an LLM-drafted one constrained by a system instruction.
- **No duplicate prevention on `published.json`** — a set-check on `comment_id`
  before writing would fix this.
- **`learning_examples.json` has no size cap** — a sliding window (keep last N)
  would prevent prompt bloat over time.
- **Single-threaded** — comments are drafted sequentially; parallelisation would
  help for larger datasets.
- **No triage step** — every comment gets a draft regardless of whether it warrants
  a reply.
