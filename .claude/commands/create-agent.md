---
description: Create a new Claude Code CLI agent using LLM-optimized prompt engineering best practices
---

# Create Claude Code CLI Agent

**Purpose:** Design and create a new specialist agent for `.claude/agents/` using systematic workflow, clarifying questions, and LLM-optimized prompt engineering.

**When to use:** User requests a new specialized agent (e.g., "create a PRD writer agent", "create a technical writer agent", "create a Rust specialist").

**What this creates:** A new `.md` file in `.claude/agents/` following LLM-optimized format (40-80 lines, dense, framework-specific).

---

## Understanding Agents vs Commands vs Skills

### Agents (Specialized Experts)
- **Invoked via Task tool** - `Task(subagent_type='agent-name', prompt='...')`
- **Specialized domain experts** - Framework-specific (react-specialist) or domain-specific (security-auditor)
- **Stored in** `.claude/agents/[name].md`
- **Format:** Markdown with YAML frontmatter, LLM-optimized (40-80 lines)
- **Purpose:** Provide deep expertise in specific framework/domain with immutable rules and quality gates
- **Customer:** LLM agents only (NOT human-readable; optimized for token efficiency)

### Commands (User-Invoked Workflows)
- **User types** `/command-name`
- **Reusable workflow templates** - Multi-phase processes (coding, investigation, debugging)
- **Stored in** `.claude/commands/[name].md`
- **Purpose:** Structure user requests with clear phases and checkpoints

### Skills (Model-Invoked Capabilities)
- **Automatically invoked** based on context
- **Stored in** `.claude/skills/[name]/`
- **Purpose:** Background processing, automatic pattern detection

**When to create an agent:**
- Need deep framework-specific expertise (react-specialist, laravel-specialist)
- Need domain-specific expertise (security-auditor, performance-profiler, api-designer)
- Task requires immutable rules and quality gates
- Task delegated via Task tool (not user-invoked)
- Want LLM-optimized prompts (short, dense, symbol-heavy)

**When to create a command instead:**
- User needs to invoke a workflow repeatedly
- Multi-phase process with checkpoints
- User-facing (human types `/command-name`)

---

## Phase 0: Initialize Agent Creation

**MANDATORY GATE: Understand requirements before designing agent.**

1. Use mcp__mcp_docker__sequentialthinking to analyze:
   - User's request: What specialist expertise is needed?
   - Agent type: Framework-specific (react, vue, laravel) or domain-specific (security, performance)?
   - Existing coverage: Is there already an agent for this? (Check `.claude/agents/`)
   - Overlap risk: Will this duplicate existing agents or commands?
   - Scope boundary: What is IN scope vs OUT of scope?
   - Success criteria: How do we know this agent is effective?

2. Use TodoWrite to track agent creation:
   - Clarify agent purpose with questions
   - Research existing agents for patterns
   - Design agent structure (sections, rules, gates)
   - Write LLM-optimized agent prompt
   - Verify structure

---

## Phase 1: Clarify Agent Purpose with Questions

**MANDATORY GATE: Ask clarifying questions to understand agent's exact role.**

Use AskUserQuestion to gather:

1. **Agent Scope:**
   - Question: "What specific expertise should this agent provide?"
   - Options:
     - Framework-specific (e.g., React, Laravel, Swift)
     - Domain-specific (e.g., security, performance, API design)
     - Project-type specific (e.g., Figma plugins, MCP servers)
     - Cross-cutting concern (e.g., testing, architecture)

2. **When Should Agent Be Used:**
   - Question: "When should this agent be invoked?"
   - Options:
     - Proactively (after every code change, like code-reviewer)
     - On-demand (when specific task arises)
     - As part of workflow (called by commands)
     - For troubleshooting (debugging, performance issues)

3. **Key Immutable Rules:**
   - Question: "What are the non-negotiable rules this agent must enforce?"
   - Gather: 3-7 core principles that NEVER change (e.g., "Type everything, no 'any'", "Validate all inputs")

4. **Quality Gates:**
   - Question: "What must be true before this agent's task is complete?"
   - Gather: Verifiable checkpoints (tests pass, bundle size, metrics)

5. **Anti-Patterns to Flag:**
   - Question: "What mistakes should this agent prevent?"
   - Gather: Common errors, bad practices, violations

**Deliverable:** Complete understanding of agent's purpose, scope, rules, gates, anti-patterns.

---

## Phase 2: Research Existing Agents for Patterns

**MANDATORY: Study existing agents before creating new one.**

### Step 1: Find Similar Agents

**Identify 2-3 similar agents to use as templates:**

```bash
# List all agents
ls -1 .claude/agents/*.md

# Read similar agents
# If creating framework-specific frontend: read react-specialist, vue-expert
# If creating framework-specific backend: read laravel-specialist, nodejs-backend-specialist
# If creating domain-specific: read security-auditor, performance-profiler, api-designer
# If creating quality/architecture: read code-reviewer, testing-architect, architecture-guardian
```

### Step 2: Identify LLM-Optimization Patterns

**All agents follow these patterns:**

1. **YAML Frontmatter**
   ```yaml
   ---
   name: agent-name
   description: One-line description (verb + object + context)
   ---
   ```

2. **Consistent Structure (40-80 lines)**
   ```markdown
   # Agent Title

   ## Mission
   [One sentence: action-oriented mission statement]

   ## Scope
   [Bullet list: technologies, frameworks, methodologies]

   ## Immutable Rules
   [Numbered list: 5-7 non-negotiable rules]

   ## Workflow
   [Numbered list: Assess→Plan→Implement→Optimize→Test→Verify]

   ## Quality Gates
   [Checkbox list: verifiable completion criteria]

   ## Anti-Patterns
   [Bullet list with ❌: common mistakes to avoid]

   ## Deliverables
   [One line: what the agent produces as proof]
   ```

3. **LLM-Optimized Formatting**
   - Dense, telegraphic style (remove articles: the, a, an)
   - Symbols (→, ✓, ❌, ×, ~)
   - Abbreviations (FKs, APIs, DB, UI, e2e)
   - Code examples ONLY if they clarify patterns
   - 40-80 line target (token efficiency)

4. **Framework-Specific Focus**
   - NO generic agents (no "frontend-specialist")
   - YES framework-specific (react-specialist, vue-expert, laravel-specialist)
   - Self-contained (no cross-referencing other agents)

**Deliverable:** List of 2-3 template agents to reference; understanding of LLM-optimization patterns.

---

## Phase 3: Design Agent Structure

**MANDATORY: Design complete structure before writing content.**

### Step 1: Define Agent Metadata

**Determine:**

1. **Agent name** (filename will be `[name].md`)
   - Use kebab-case: `rust-specialist.md`, `prd-writer.md`
   - Descriptive, specific (not generic)
   - Matches Task tool invocation: `Task(subagent_type='rust-specialist')`

2. **Description** (for frontmatter)
   - One line, max 100 characters
   - Format: `[Verb] + [object] + [context/constraints]`
   - Examples:
     - "Rust specialist. Builds safe, fast systems with ownership, lifetimes, zero-cost abstractions."
     - "PRD writer. Creates detailed product requirements with user stories, acceptance criteria, metrics."

3. **Mission statement**
   - One sentence, action-oriented
   - Examples:
     - "Ship memory-safe, concurrent Rust code using ownership, lifetimes, zero-cost abstractions."
     - "Write clear, actionable PRDs with user stories, acceptance criteria, success metrics, technical constraints."

### Step 2: Define Scope (Technologies/Methodologies)

**List 5-10 key areas of expertise:**
- Technologies (Rust, Cargo, async/await)
- Frameworks (Tokio, Actix, Axum)
- Methodologies (ownership model, borrowing, lifetimes)
- Tools (rustc, clippy, cargo, rustfmt)
- Patterns (Result/Option, error handling, traits)

### Step 3: Define Immutable Rules (5-7 Core Principles)

**Format:** Numbered list, starts with imperative verb

**Examples:**
```markdown
1) Use Result<T, E> for recoverable errors; panic! only for unrecoverable.
2) Leverage ownership; avoid clone() without justification (measure cost).
3) Type everything; no unsafe without documented safety invariants.
```

### Step 4: Define Workflow (5-6 Steps)

**Format:** Assess→Plan→Implement→Optimize→Test→Verify

**Use arrow notation (→) for flow:**
```markdown
1. Assess→requirements, constraints, performance needs
2. Plan→module structure, types, error handling strategy
3. Implement→typed code, tests, error handling
4. Optimize→profiling, benchmarks, zero-cost abstractions
5. Test→unit tests, integration tests, property tests
6. Verify→clippy clean, tests pass, benchmarks meet targets
```

### Step 5: Define Quality Gates (Verifiable Checkpoints)

**Format:** Checkbox list with ✓

**Examples:**
```markdown
- ✓ Zero clippy warnings; rustfmt applied
- ✓ All functions return Result<T, E> or Option<T>; no panic!
- ✓ Tests pass; coverage >85%; property tests for core logic
- ✓ Benchmarks meet targets; profiling shows no hotspots
```

### Step 6: Define Anti-Patterns (5-10 Common Mistakes)

**Format:** Bullet list with ❌

**Examples:**
```markdown
- ❌ clone() everywhere (ownership not leveraged; performance cost)
- ❌ unwrap() in production (panics on None/Err; use ? or match)
- ❌ unsafe without safety comments (invariants undocumented)
```

### Step 7: Define Deliverables (One Line)

**Format:** "Short plan, changed files, proof: [specific evidence]"

**Examples:**
```markdown
Short plan, changed files, proof: clippy clean, tests pass, benchmarks meet targets.
```

**Deliverable:** Complete agent structure outline with all sections defined.

---

## Phase 4: Write LLM-Optimized Agent Prompt

**MANDATORY: Follow LLM-optimization principles for token efficiency.**

### LLM-Optimization Principles

**Research findings (2025):**
- Dense, symbol-based formats save 30-40% tokens vs prose
- Shorter prompts = better LLM adherence
- Custom notation outperforms JSON/YAML for conciseness
- Self-contained agents (no cross-refs) = better context efficiency

**Apply these techniques:**

1. **Remove Articles**
   - ❌ "Use the ownership model for the memory safety"
   - ✅ "Use ownership model for memory safety"

2. **Telegraphic Style**
   - ❌ "You should validate all user inputs at the API boundary using schemas"
   - ✅ "Validate all inputs at API boundary; use schemas"

3. **Symbols Over Words**
   - Use →, ✓, ❌, ~, ×, <, >, ≤, ≥
   - Examples: `Assess→Plan→Implement` instead of "First assess, then plan, then implement"

4. **Abbreviations**
   - API, DB, UI, UX, e2e, FK (foreign key), ORM, SQL, TS, JS

5. **Dense Lists**
   - Semicolons to separate items: "Use Result; avoid panic!; document unsafe"
   - Parenthetical context: "Tests pass (unit, integration, e2e)"

6. **Code Examples ONLY If Clarifying**
   - Include code ONLY if it demonstrates a pattern better than prose
   - Keep examples <10 lines
   - Use comments sparingly

7. **40-80 Line Target**
   - Most agents: 50-60 lines
   - Complex agents (with code examples): up to 80 lines
   - Simple agents: minimum 40 lines

### Writing Process

**Step 1: Write Header**
```markdown
---
name: [agent-name]
description: [One-line description]
---

# [Agent Title]

## Mission
[One sentence mission]
```

**Step 2: Write Scope (Dense List)**
```markdown
## Scope
- [Category]: [items, items, items]
- [Category]: [items]; [subcategory items]
```

**Step 3: Write Immutable Rules (Imperative Numbered List)**
```markdown
## Immutable Rules
1) [Rule with constraints/context in parens or semicolons]
2) [Rule]; [clarification]; [exception if any].
```

**Step 4: Write Workflow (Arrow Notation)**
```markdown
## Workflow
1. Assess→[aspects to evaluate]
2. Plan→[what to design]
3. Implement→[what to build]
4. Optimize→[what to improve]
5. Test→[what to verify]
6. Verify→[final checks]
```

**Step 5: Write Quality Gates (Checkboxes)**
```markdown
## Quality Gates
- ✓ [Verifiable condition]; [metric/tool]
- ✓ [Verifiable condition]; [evidence required]
```

**Step 6: Write Anti-Patterns (❌ List)**
```markdown
## Anti-Patterns
- ❌ [Anti-pattern] ([reason/consequence])
- ❌ [Anti-pattern]; [correct approach instead]
```

**Step 7: Write Deliverables (One Line)**
```markdown
## Deliverables
Short plan, changed files, proof: [specific evidence], [metrics], [verification].
```

**Step 8: Optional Code Examples**
Only add if necessary to clarify patterns:
```markdown
## [Framework] Patterns (OPTIONAL)
\```language
// Brief example (max 10 lines)
\```
```

### Step 9: Optimize for Token Efficiency

**Review and compress:**
1. Count lines: Target 40-80, ideal 50-60
2. Remove redundant words: "in order to" → "to"
3. Use symbols: "and then" → "→"
4. Compress lists: "X, Y, and Z" → "X, Y, Z"
5. Remove filler: "It is important to" → "" (implied by Immutable Rules)

**Deliverable:** Complete `.claude/agents/[name].md` file, LLM-optimized, 40-80 lines.

---

## Phase 5: Verify  Agent

**MANDATORY GATE: Verify structure before claiming complete.**

### Step 1: Structural Verification

**Check file structure:**
```bash
# Verify frontmatter
head -5 .claude/agents/[name].md

# Count lines (should be 40-80)
wc -l .claude/agents/[name].md

# Verify sections present
grep "^## " .claude/agents/[name].md
```

**Required sections:**
- ✓ YAML frontmatter (name, description)
- ✓ Mission (one sentence)
- ✓ Scope (bulleted list)
- ✓ Immutable Rules (5-7 numbered items)
- ✓ Workflow (5-6 steps with →)
- ✓ Quality Gates (checkboxes with ✓)
- ✓ Anti-Patterns (❌ bullets)
- ✓ Deliverables (one line)

### Step 2: Content Quality Checks

**Verify agent quality:**

**Mission:**
- [ ] One sentence, action-oriented
- [ ] Starts with verb (Ship, Build, Design, Write)
- [ ] Mentions key technologies/constraints

**Scope:**
- [ ] 5-10 items covering key areas
- [ ] Framework/tool names specific (not generic)
- [ ] Dense format (semicolons, commas)

**Immutable Rules:**
- [ ] 5-7 rules (not more, not less)
- [ ] Each starts with imperative verb
- [ ] Specific, actionable (not vague)
- [ ] Non-negotiable (these NEVER change)

**Workflow:**
- [ ] 5-6 steps (consistent pattern)
- [ ] Uses arrow notation (→)
- [ ] Assess→Plan→Implement→Optimize→Test→Verify flow
- [ ] Each step clear about what to do

**Quality Gates:**
- [ ] All verifiable (not subjective)
- [ ] Include tools/metrics (clippy, tests, benchmarks)
- [ ] ✓ checkboxes used
- [ ] Specific thresholds (>85%, <10ms, etc.)

**Anti-Patterns:**
- [ ] 5-10 common mistakes
- [ ] ❌ symbols used
- [ ] Reason/consequence in parens
- [ ] Actionable (what NOT to do)

**Deliverables:**
- [ ] One line format: "Short plan, changed files, proof: [evidence]"
- [ ] Specific evidence mentioned (test output, metrics, screenshots)

**Token Efficiency:**
- [ ] 40-80 lines total
- [ ] Articles removed (the, a, an)
- [ ] Symbols used (→, ✓, ❌)
- [ ] Abbreviations used (API, DB, UI, etc.)
- [ ] No redundant prose
- [ ] Code examples only if clarifying (max 10 lines each)

---

## Good vs Bad Agent Examples

### Example 1: Framework-Specific Agent

**❌ BAD (Generic, Verbose, 200+ Lines):**
```markdown
---
name: frontend-specialist
description: Frontend development specialist for various frameworks
---

# Frontend Specialist

## Mission
This agent helps with all kinds of frontend development across multiple frameworks
including React, Vue, Angular, Svelte, and more. The agent will provide guidance
on best practices, performance optimization, and modern patterns.

[Continues for 200+ lines with conditional "if React do X, if Vue do Y" logic]
```

**Problems:**
- Too generic ("various frameworks")
- Conditional logic ("if React... if Vue...")
- Verbose prose style
- 200+ lines (poor LLM adherence)
- Not self-contained

**✅ GOOD (Framework-Specific, Dense, 56 Lines):**
```markdown
---
name: react-specialist
description: React 18+ specialist. Builds typed, performant apps with hooks, RSC, Server Actions, Suspense.
---

# React Specialist (18+)

## Mission
Ship fast, maintainable React using TypeScript, hooks, Server Components, measured performance.

## Scope
- Hooks: useState/useEffect/useCallback/useMemo/useTransition; custom hooks
- RSC: Server Components, Client Components, Server Actions, streaming
- Suspense: lazy loading, data fetching boundaries, error boundaries
- TypeScript: props/state/refs/context/hooks typed; no any

[Continues for 56 lines, all React-specific]
```

**Why it's good:**
- Framework-specific (React 18+)
- Dense, telegraphic style
- 56 lines (optimal LLM adherence)
- Self-contained (no cross-refs)
- Actionable rules

### Example 2: Domain-Specific Agent

**❌ BAD (Vague, Missing Quality Gates):**
```markdown
---
name: security-helper
description: Helps with security
---

# Security Helper

## Mission
Help make code more secure

## Scope
- Security stuff
- Vulnerabilities
- Best practices

## Rules
1) Be secure
2) Check for issues
3) Fix problems

[No Quality Gates, no Anti-Patterns, no Deliverables]
```

**Problems:**
- Vague ("security stuff", "be secure")
- No specific tools/methodologies
- Missing Quality Gates
- Not actionable
- Too short (<30 lines, incomplete)

**✅ GOOD (Specific, Actionable, 57 Lines):**
```markdown
---
name: security-auditor
description: Security specialist. Audits for OWASP Top 10, auth flaws, injection, secrets leaks, XSS, CSRF.
---

# Security Auditor

## Mission
Identify and fix security vulnerabilities using OWASP Top 10, secure coding practices, defense-in-depth.

## Scope
- OWASP Top 10 2025: Broken Access Control, Cryptographic Failures, Injection, etc.
- Injection: SQL, NoSQL, Command, LDAP; parameterized queries
- XSS: reflected, stored, DOM-based; CSP, output encoding
- Auth: password hashing (argon2/bcrypt), JWTs, MFA, RBAC

[Continues with specific rules, gates, anti-patterns for 57 lines]
```

**Why it's good:**
- Specific scope (OWASP Top 10 named)
- Tools mentioned (argon2, bcrypt, CSP)
- Quality Gates with verifiable checks
- Anti-Patterns with ❌
- Complete Deliverables

---

## Quality Checklist

**Before claiming agent creation complete, verify ALL items:**

### Structure (Required Elements)
- [ ] YAML frontmatter with `name` and `description`
- [ ] Mission section (one sentence)
- [ ] Scope section (5-10 items)
- [ ] Immutable Rules section (5-7 numbered rules)
- [ ] Workflow section (5-6 steps with →)
- [ ] Quality Gates section (checkboxes with ✓)
- [ ] Anti-Patterns section (❌ bullets)
- [ ] Deliverables section (one line)

### Content Quality
- [ ] Mission is action-oriented (starts with verb)
- [ ] Scope is specific (framework/tool names, not generic)
- [ ] Immutable Rules are non-negotiable, actionable
- [ ] Workflow follows Assess→Plan→Implement→Optimize→Test→Verify
- [ ] Quality Gates are verifiable (metrics, tools, thresholds)
- [ ] Anti-Patterns include reason/consequence
- [ ] Deliverables specify evidence (tests, metrics, screenshots)

### LLM-Optimization
- [ ] 40-80 lines total (ideal: 50-60)
- [ ] Articles removed (the, a, an)
- [ ] Symbols used (→, ✓, ❌)
- [ ] Abbreviations used (API, DB, UI)
- [ ] Telegraphic style (no prose)
- [ ] Code examples only if clarifying (<10 lines each)
- [ ] Self-contained (no cross-references to other agents)

### Specificity
- [ ] Framework-specific OR domain-specific (not generic)
- [ ] Agent name is descriptive ([framework]-specialist or [domain]-auditor)
- [ ] No conditional logic ("if X then Y, if Z then W")
- [ ] All rules apply to THIS framework/domain only

### Integration
- [ ] Correct category (Frontend, Backend, Domain, Quality, Project-Type)
- [ ] Description matches frontmatter
- [ ] "When to use" column is clear

### Verification
- [ ] File created in `.claude/agents/[name].md`
- [ ] Line count verified (40-80 lines)
- [ ] All sections present (grep "^## ")
- [ ] Frontmatter valid YAML
- [ ] (Optional) Test invocation successful

**RED FLAGS (fix before completing):**
- Generic agent name (frontend-specialist, backend-helper)
- Vague mission ("help with X")
- Conditional logic ("if React do X, if Vue do Y")
- >80 lines OR <40 lines
- Missing Quality Gates or Anti-Patterns
- Prose style (not telegraphic)
- Cross-references to other agents

---

## Final Statement Template

**After ALL checklist items verified:**

"Agent creation complete. Evidence:
- [Agent file path and line count]
- [Sections verified present]

**Remember: LLM-optimized agents are customer-focused on LLMs, not humans. Prioritize token efficiency, density, and actionability over human readability.**
