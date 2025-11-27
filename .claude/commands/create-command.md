---
description: Create a new Claude Code CLI command using prompt engineering best practices
---

# Create Claude Code CLI Command

**Purpose:** Design and create a new slash command for Claude Code CLI using systematic workflow and prompt engineering best practices.

**When to use:** User requests a new repeatable workflow that should be encapsulated as a slash command.

**What this creates:** A new `.md` file in `.claude/commands/` that follows established patterns and integrates with the project's ecosystem.

---

## Understanding Claude Code CLI Commands

### Commands vs Skills

**Commands (Slash Commands):**
- **User-invoked** - User types `/command-name` to trigger
- **Reusable prompt templates** - Encapsulate workflows
- **Stored in** `.claude/commands/[name].md`
- **Format:** Markdown with YAML frontmatter
- **Purpose:** Help users structure requests with clear workflow and expectations

**Skills (Agent Skills):**
- **Model-invoked** - I decide when to use based on context
- **Specialized capabilities** - Domain-specific expertise
- **Stored in** `.claude/skills/[name]/`
- **Purpose:** Specialized agents for specific tasks

**Use commands when:**
- User needs to invoke a specific workflow repeatedly
- Workflow has clear phases/steps
- User wants to structure their request
- Multiple people on team need same workflow

**Use skills when:**
- Automatic invocation based on context
- Specialized domain knowledge needed
- Background processing required

### Command File Format

```markdown
---
description: Short one-line description (shows in /help)
---

# Command Title

**Purpose:** [What this command does]

**When to use:** [Scenarios for using this command]

[Rest of command content...]
```

### How Commands Work

1. User types `/command-name` or `/command-name arguments`
2. Claude Code reads `.claude/commands/command-name.md`
3. The markdown content becomes my prompt
4. I execute the workflow described in the command
5. User sees my responses (command content is hidden from user)

**Important:** The command content is my instructions, not shown to user.

---

## Phase 0: Initialize Command Creation

**MANDATORY GATE: Understand the request before designing the command.**

1. Use mcp__mcp_docker__sequentialthinking to analyze:
   - User's request: What workflow are they trying to encapsulate?
   - Target audience: Who will use this command?
   - Frequency: How often will it be used?
   - Complexity: Simple (3-5 steps) or complex (10+ phases)?
   - Integration needs: Which tools, skills, or other commands does it need?
   - Success criteria: How do we know the command worked?

2. Use TodoWrite to track command creation:
   - Research existing commands for patterns
   - Design command structure
   - Write command content
   - Test command
   - Document command

---

## Phase 1: Research Existing Command Patterns

**MANDATORY: Study existing commands before creating new one.**

### Step 1: Read All Existing Commands

**Read these files to understand patterns:**
```
.claude/commands/code.md — Coding workflow (10 phases)
.claude/commands/investigate.md — Bug investigation
.claude/commands/debug.md — Agent-driven debugging
.claude/commands/vibecheck.md — Code quality audit
.claude/commands/codereview.md — Style guide enforcement
.claude/commands/ci.md — Autonomous improvement
.claude/commands/bootstrap-docs.md — Documentation generation
```

### Step 2: Identify Common Patterns

**All commands share these patterns:**

1. **YAML Frontmatter**
   ```yaml
   ---
   description: Short description
   ---
   ```

2. **Phase 0: Initialize**
   - Use `mcp__mcp_docker__sequentialthinking` to analyze scope
   - Use `TodoWrite` to create tasks for all phases

3. **MANDATORY GATE checkpoints**
   ```markdown
   **MANDATORY GATE: Do X before proceeding.**
   ```

4. **Phase structure with clear deliverables**
   ```markdown
   ## Phase N — Title

   [Instructions]

   **Deliverable:** [What this phase produces]
   ```

5. **Integration with project ecosystem:**
   - References to `docs/architecture.md` for project structure
   - CIB-003 documentation discipline in documentation phases
   - Standard artifact locations (`screenshots/`, `tests/`, etc.)
   - verification-before-completion pattern in final phases

6. **Verification before completion:**
   - Explicit commands to run
   - Evidence-based completion checklists
   - RED FLAGS list
   - Final statement template

7. **Forbidden patterns:**
   ```markdown
   **FORBIDDEN:**
   - ❌ [Anti-pattern]
   - ❌ [Anti-pattern]
   ```

### Step 3: Identify Integration Points

**Commands integrate with:**

1. **CLAUDE.md (Project Instructions)**
   - CIB-001: Mandatory workflow
   - CIB-002: Investigation-first development
   - CIB-003: Documentation discipline
   - TEST DATABASE SAFETY rules
   - TEST MEMORY MANAGEMENT rules
   - Standard folder structure

2. **Skills (Model-Invoked)**
   - verification-before-completion
   - test-driven-development
   - testing-anti-patterns
   - defense-in-depth
   - root-cause-tracing

3. **Tools (Must Use)**
   - `mcp__mcp_docker__sequentialthinking` — Analysis and planning
   - `TodoWrite` — Task tracking
   - `Read` — Reading files
   - `Grep` / `Glob` — Searching codebase
   - `Bash` — Running commands
   - `Task` — Delegating to specialized agents

4. **Documentation References**
   - `docs/architecture.md` — Project structure, base classes
   - `docs/api.md` — API documentation
   - `docs/database.md` — Database schema
   - `docs/testing.md` — Testing strategy
   - `docs/deployment.md` — Deployment process

5. **Standard Artifact Locations**
   - `screenshots/` — Screenshots
   - `test-results/` — Test output
   - `tests/` — All tests
   - `database/migrations/` — Migrations
   - `docs/` — Documentation (canonical only)

---

## Phase 2: Design Command Structure

**MANDATORY: Design complete structure before writing content.**

### Step 1: Define Command Metadata

**Determine:**

1. **Command name** (filename will be `[name].md`)
   - Use kebab-case: `my-command.md`
   - Descriptive but concise
   - Easy to type (users will type `/my-command`)

2. **Description** (for frontmatter)
   - One line, max 80 characters
   - Describes what the command does
   - Shows in `/help` output

3. **Purpose statement**
   - 1-2 sentences
   - What does this command accomplish?
   - When should it be used?

### Step 2: Identify Required Phases

**Map out the workflow phases:**

**Every command should have:**
- Phase 0: Initialize (sequential thinking + TodoWrite)
- Phase N: Final Verification (evidence-based completion)

**Between Phase 0 and final, identify:**
- Discovery/Investigation phases
- Planning phases
- Execution phases
- Verification phases
- Documentation phases (if creates/changes code)

**Example phase structure:**

**Simple command (3-5 steps):**
```
Phase 0: Initialize
Phase 1: [Main action]
Phase 2: Verification
```

**Complex command (like /code):**
```
Phase 0: Initialize
Phase 1: Understand Requirements
Phase 2: Discover Existing Code
Phase 3: Analyze Impact
Phase 4: Design Solution
Phase 5: Implement
Phase 6: Verify
Phase 7: Document
Phase 8: Final Verification
```

### Step 3: Identify Gate Functions

**For each phase, determine:**

- Does this phase have prerequisites? → Add MANDATORY GATE
- Are there common mistakes? → Add RED FLAGS
- Should user STOP before proceeding? → Add STOP checkpoint
- Is verification needed? → Add verification checklist

**Gate function patterns:**

```markdown
**MANDATORY GATE: Complete X before proceeding to Y.**

**STOP - Do not [action] yet**

1. Check: "[Question]?"

   IF [condition] → [action]
   IF NOT → [alternative action]

**RED FLAGS (indicate [problem]):**
- [Pattern 1]
- [Pattern 2]
```

### Step 4: Identify Tool Usage

**For this command, which tools are needed?**

**Always required:**
- `mcp__mcp_docker__sequentialthinking` in Phase 0
- `TodoWrite` in Phase 0

**Common tools:**
- `Read` — Reading files (architecture docs, code)
- `Grep` / `Glob` — Searching codebase
- `Bash` — Running commands (tests, build, migrations)
- `Edit` / `Write` — Modifying/creating files
- `Task` — Delegating to specialized agents

**When to use Task tool with agents:**
- Complex debugging → `debugger` agent
- Code review needed → `code-reviewer` agent
- Architecture changes → `architecture-guardian` agent
- UI/UX work → `ui-polish-specialist` agent
- Testing work → `testing-architect` agent

### Step 5: Define Deliverables

**For each phase, what is produced?**

Examples:
- "Task Summary + Acceptance Criteria"
- "Reuse Proof listing existing code to leverage"
- "Impact Matrix with affected files"
- "Test output showing 0 failures"
- "Screenshot of working feature"

### Step 6: Define Success Criteria

**How do we know the command succeeded?**

**Final verification checklist should include:**
- Explicit commands run with passing output
- Artifacts created in correct locations
- No forbidden patterns (per CIB-003 if docs involved)
- Evidence-based completion proof

**Example:**
```markdown
**Completion Checklist (ALL must be TRUE):**
- [ ] Tests pass (0 failures)
- [ ] Screenshots saved to screenshots/
- [ ] Documentation updated per CIB-003
- [ ] No summary/report documents created
- [ ] Evidence: [specific evidence required]
```

---

## Phase 3: Write Command Content

**MANDATORY: Follow established patterns and voice.**

### Step 1: Write YAML Frontmatter

```yaml
---
description: [One-line description, max 80 chars]
---
```

### Step 2: Write Command Header

```markdown
# [Command Title]

**Purpose:** [1-2 sentences - what this accomplishes]

**When to use:** [Scenarios for using this command]
- [Scenario 1]
- [Scenario 2]

**What this creates/does:** [Expected outcomes]
```

### Step 3: Write Phase 0 (Initialize)

**Template:**
```markdown
## Phase 0: Initialize

**MANDATORY GATE: Complete [analysis/understanding] before proceeding.**

1. Use mcp__mcp_docker__sequentialthinking to analyze:
   - [Aspect 1 to analyze]
   - [Aspect 2 to analyze]
   - [Aspect 3 to analyze]

2. Use TodoWrite to create tasks for all phases:
   1. Phase 1 — [Title]
   2. Phase 2 — [Title]
   3. Phase N — [Title]
```

### Step 4: Write Each Workflow Phase

**For each phase, use this structure:**

```markdown
## Phase N — [Descriptive Title]

**MANDATORY GATE:** [If applicable - what must be done before this phase]

[Introduction - what this phase does]

### Step 1: [First step]

[Detailed instructions]

**If using tools:**
- Use [Tool] to [action]
- Check [condition]
- Verify [outcome]

### Step 2: [Second step]

[Instructions]

**Deliverable:** [What this phase produces]

**RED FLAGS:** [If applicable - patterns to avoid]
- [Pattern 1]
- [Pattern 2]
```

### Step 5: Write Documentation Phase (if applicable)

**If command creates/modifies code, include:**

```markdown
## Phase N — Documentation (Following CIB-003)

**MANDATORY GATE: Follow CIB-003 documentation discipline.**

**BEFORE creating or editing ANY .md file:**

```
STOP - Do not touch any .md file yet

1. Check: "Is this file in the canonical documentation registry?"
   - docs/architecture.md
   - docs/api.md
   - docs/database.md
   - docs/testing.md
   - docs/deployment.md

   IF YES → Proceed to update it
   IF NO → ASK user permission first

2. Check: "Does this involve dated/summary/status/analysis content?"
   IF YES → STOP → Update canonical doc instead
```

**Documentation Tasks:**
1. **Update canonical documentation:**
   - If [condition] → Update `docs/[file].md`

2. **Update inline documentation:**
   - Add/update PHPDoc/TSDoc

**FORBIDDEN:**
- ❌ Creating `docs/[SUMMARY_REPORT].md`
- ❌ Any dated documentation
```

### Step 6: Write Final Verification Phase

**Template:**
```markdown
## Phase N — Final Verification (Before Claiming Complete)

**MANDATORY GATE: verification-before-completion skill applies here.**

**NO COMPLETION CLAIMS without running these commands and seeing PASS:**

**[Backend/Frontend/Full] Verification:**
```bash
# [Command category]
[actual command to run]

# [Another command]
[actual command to run]
```

**Completion Checklist (ALL must be TRUE):**
- [ ] [Specific verifiable item]
- [ ] [Tests pass with 0 failures]
- [ ] [Evidence captured (screenshots/logs)]
- [ ] [Documentation updated per CIB-003]
- [ ] [No forbidden patterns created]
- [ ] [Specific success criteria met]

**RED FLAGS (require fixing before claiming complete):**
- [Pattern indicating incomplete work]
- [Pattern indicating skipped steps]
- [Pattern indicating quality issues]

**Final Statement Template:**
"[Task type] complete. Evidence:
- [Command output showing success]
- [Screenshot/artifact location]
- [Documentation updated]"

Remember: Evidence before claims. Always.
```

### Step 7: Apply Consistent Voice and Style

**Voice characteristics:**

1. **Imperative mood**
   - "Use [tool]" not "You should use [tool]"
   - "Run [command]" not "Please run [command]"
   - "STOP" not "Please stop"

2. **Strong absolute language**
   - MANDATORY, NEVER, ALWAYS, MUST
   - "NO [action] without [condition]"
   - "All [items] MUST [requirement]"

3. **Structured format**
   - Numbered steps for sequences
   - Bullet points for lists
   - Code blocks for commands
   - Tables for comparisons

4. **Explicit anti-patterns**
   - List what NOT to do
   - Mark with ❌
   - Explain why it's forbidden

5. **Evidence-based language**
   - "Evidence before claims"
   - "Run [command] and see [output]"
   - "Capture proof with [artifact]"

**Example good voice:**
```markdown
**MANDATORY: Complete ALL discovery before writing ANY code.**

Use Grep to search for existing implementations:
- Search `app/Domain/*` for similar functionality
- Search `app/Support/*` for reusable utilities
- Document findings in TodoWrite

**FORBIDDEN:**
- ❌ Writing code without searching for existing implementations
- ❌ Duplicating functionality that already exists

**RED FLAG:** If creating new helper without checking for existing → STOP → Go back to discovery.
```

**Example poor voice:**
```markdown
You should probably search for existing code before writing new functionality. This helps avoid duplication.

Try using Grep to look around the codebase. You might find something useful.

It's generally not a good idea to duplicate code, so please be careful about that.
```

---

## Phase 4: Integrate with Project Ecosystem

**MANDATORY: Ensure command works with existing infrastructure.**

### Step 1: Reference Architecture Documentation

**If command involves code/structure, include:**

```markdown
**FIRST: Read `docs/architecture.md`** to understand:
- Current project structure
- Base classes and traits
- Architectural patterns
```

**Use generic language + doc reference:**
- ✅ "Check `docs/architecture.md` for base classes"
- ❌ "Extend `App\Http\Controllers\BaseController`" (might change)

### Step 2: Reference Standard Locations

**Always use standard framework paths:**
```markdown
**Standard framework locations:**
- `tests/` — All tests
- `database/migrations/` — Database migrations
- `screenshots/` — Screenshots
- `app/Models/` — Eloquent models
- `routes/*.php` — Route definitions
```

**Reference docs for project-specific:**
```markdown
**Project-specific locations** (from `docs/architecture.md`):
- Business logic (check architecture doc)
- Domain structure (check architecture doc)
```

### Step 3: Integrate CIB-003 (Documentation Discipline)

**If command touches documentation:**

1. Include CIB-003 gate function
2. Reference canonical documentation registry
3. Forbid summary/status/analysis documents
4. Include verification checklist for docs

### Step 4: Use Sequential Thinking Appropriately

**Include sequential thinking for:**
- Complex analysis phases
- Decision-making points
- Impact analysis
- Root cause investigation

**Pattern:**
```markdown
Use mcp__mcp_docker__sequentialthinking to [analyze/discuss/investigate]:
- [Aspect 1]
- [Aspect 2]
```

### Step 5: Use TodoWrite for Tracking

**Always in Phase 0:**
```markdown
Use TodoWrite to create tasks for all phases:
```

**Optionally in later phases:**
```markdown
Add findings to TodoWrite:
- [Finding 1]
- [Finding 2]
```

### Step 6: Reference Skills When Appropriate

**Link to existing skills:**
```markdown
**MANDATORY GATE: verification-before-completion skill applies here.**
```

**Or:**
```markdown
Follow test-driven-development skill:
1. Write failing test
2. Watch it fail
3. Write minimal code to pass
```

---

## Phase 5: Add Examples and Templates

**MANDATORY: Provide concrete examples to prevent ambiguity.**

### Step 1: Add Command Examples

**Show what commands to run:**

```markdown
**Backend Verification:**
```bash
# Code style
./vendor/bin/pint --test

# Tests
php artisan test
```
```

### Step 2: Add Request/Response Examples

**If command involves API/data:**

```markdown
**Request:**
```json
{
  "field": "value"
}
```

**Response (200):**
```json
{
  "error": false,
  "data": {...}
}
```
```

### Step 3: Add Code Examples

**If command involves code patterns:**

```markdown
**Example implementation:**
```php
class ExampleController extends BaseApiController
{
    public function index()
    {
        // Implementation
    }
}
```
```

### Step 4: Add Decision Tree Examples

**For complex decisions:**

```markdown
**Decision tree:**
```
IF creating new controller
  → Check docs/architecture.md for base controller
  → Extend base controller
  → Follow naming convention

IF modifying existing controller
  → Check for side effects
  → Update tests
```
```

### Step 5: Add Final Statement Template

**Always include completion template:**

```markdown
**Final Statement Template:**
"[Task] complete. Evidence:
- [Specific evidence 1]
- [Specific evidence 2]
- [Specific evidence 3]"
```

---

## Phase 6: Review and Refine

**MANDATORY: Review command before considering it complete.**

### Step 1: Self-Review Checklist

**Structure:**
- [ ] YAML frontmatter with description
- [ ] Clear title and purpose statement
- [ ] Phase 0 with sequential thinking + TodoWrite
- [ ] Clear phases with descriptive titles
- [ ] Final verification phase with evidence requirements

**Patterns:**
- [ ] MANDATORY GATE checkpoints where needed
- [ ] RED FLAGS for common mistakes
- [ ] FORBIDDEN patterns explicitly listed
- [ ] Standard artifact locations referenced
- [ ] Documentation discipline (CIB-003) if applicable

**Integration:**
- [ ] References `docs/architecture.md` for project structure
- [ ] Uses standard framework paths
- [ ] Integrates with TodoWrite
- [ ] Integrates with sequential thinking
- [ ] Links to relevant skills

**Clarity:**
- [ ] Imperative voice throughout
- [ ] Concrete examples provided
- [ ] Commands explicitly shown
- [ ] Success criteria unambiguous
- [ ] Anti-patterns explicitly forbidden

**Completeness:**
- [ ] All phases have deliverables
- [ ] Verification requirements explicit
- [ ] Final statement template included
- [ ] No TODO or placeholder sections

### Step 2: Test Command Logic

**Walk through mentally:**

1. **If I follow this command exactly, what happens?**
   - Can I complete each phase?
   - Are instructions clear?
   - Are there ambiguous decisions?

2. **What could go wrong?**
   - Missing tools?
   - Unclear conditions?
   - Circular references?

3. **Does it integrate properly?**
   - Would `docs/architecture.md` exist when this runs?
   - Are file paths correct?
   - Do referenced commands work?

### Step 3: Check for Common Mistakes

**Avoid these anti-patterns:**

❌ **Vague instructions**
```markdown
Search for relevant code
```
✅ **Specific instructions**
```markdown
Use Grep to search `app/Domain/*` for existing implementations
```

❌ **Aspirational documentation**
```markdown
Update the comprehensive API documentation
```
✅ **Reality-based documentation**
```markdown
Update `docs/api.md` with new endpoint (following CIB-003)
```

❌ **Missing verification**
```markdown
Make sure tests pass
```
✅ **Explicit verification**
```markdown
Run `php artisan test` and verify 0 failures:
- [ ] Command output shows "Tests: X passed"
```

❌ **Weak gates**
```markdown
You should probably check existing code first
```
✅ **Strong gates**
```markdown
**MANDATORY GATE: DO NOT proceed without completing discovery.**
```

❌ **Hardcoded project paths**
```markdown
Check `app/Services/UserService.php`
```
✅ **Generic + doc reference**
```markdown
Check `docs/architecture.md` for service layer location
```

### Step 4: Refine Language

**Strengthen weak language:**

| Weak | Strong |
|------|--------|
| "You should" | "Use" or "MUST" |
| "Try to" | "Do" or "Complete" |
| "It's better if" | "ALWAYS" |
| "Don't forget" | "NEVER skip" |
| "Probably" | Remove uncertainty |
| "Maybe check" | "Check" or "Verify" |

**Remove hedging:**
- "might want to"
- "could be good"
- "perhaps consider"

**Add clarity:**
- Numbered steps over paragraphs
- Tables over prose
- Code blocks over descriptions
- Examples over explanations

---

## Phase 7: Save and Document Command

**MANDATORY: Save command and verify it works.**

### Step 1: Save Command File

**File location:** `.claude/commands/[command-name].md`

**Filename requirements:**
- Use kebab-case
- Match intended slash command
- Descriptive but concise
- Example: `analyze-performance.md` → `/analyze-performance`

**Save the file:**
```markdown
Use Write tool to create:
.claude/commands/[command-name].md

With content:
[Full command content created in Phase 3-6]
```

### Step 2: Verify File Format

**Check:**
- [ ] File saved in `.claude/commands/`
- [ ] Filename is kebab-case
- [ ] Extension is `.md`
- [ ] YAML frontmatter present
- [ ] Content follows markdown format

### Step 3: Test Command Reference

**Verify command can be invoked:**
```markdown
The command can be invoked with:
/[command-name]

Or with arguments:
/[command-name] [arguments]
```

### Step 4: Add to Project Documentation

**Do NOT create summary document.**

**Instead, update `docs/architecture.md` if command introduces new patterns:**

```markdown
## Available Commands

See `.claude/commands/` for all commands.

Key commands:
- `/code` - Coding workflow
- `/investigate` - Bug investigation
- `/[new-command]` - [Brief description]
```

**Or simply note:**
```markdown
New command `/[command-name]` available.
Use `/help` to see all commands.
```

---

## Phase 8: Final Verification

**MANDATORY GATE: Verify command quality before claiming complete.**

### Complete Command Quality Checklist

**File & Format:**
- [ ] File created at `.claude/commands/[name].md`
- [ ] YAML frontmatter with description
- [ ] Markdown format correct
- [ ] No syntax errors

**Structure:**
- [ ] Phase 0: Initialize (sequential thinking + TodoWrite)
- [ ] Clear workflow phases
- [ ] Final verification phase
- [ ] Each phase has purpose and deliverable

**Patterns (from existing commands):**
- [ ] MANDATORY GATE checkpoints
- [ ] RED FLAGS for anti-patterns
- [ ] FORBIDDEN patterns listed
- [ ] Evidence-based completion
- [ ] Final statement template

**Integration:**
- [ ] References `docs/architecture.md` for structure
- [ ] Uses standard framework paths
- [ ] CIB-003 if documentation involved
- [ ] TodoWrite in Phase 0
- [ ] Sequential thinking in Phase 0
- [ ] Appropriate tool usage

**Clarity:**
- [ ] Imperative voice throughout
- [ ] No vague instructions
- [ ] Concrete examples provided
- [ ] Commands explicitly shown with ```bash blocks
- [ ] Decision trees clear

**Completeness:**
- [ ] All phases complete
- [ ] No TODO or placeholder text
- [ ] Success criteria explicit
- [ ] Verification requirements clear
- [ ] Examples provided

**No Anti-Patterns:**
- [ ] No aspirational documentation
- [ ] No hardcoded project-specific paths (use docs reference)
- [ ] No weak language ("should", "try to", "maybe")
- [ ] No missing verifications
- [ ] No summary document creation

**FORBIDDEN (verify NOT created):**
- ❌ No `docs/COMMAND_CREATION_SUMMARY.md`
- ❌ No analysis or report documents
- ❌ No dated documentation

---

## Examples: Good vs Bad Commands

### Example 1: Gate Functions

**❌ Bad (weak):**
```markdown
Before writing code, you should search for existing implementations.
```

**✅ Good (strong):**
```markdown
**MANDATORY GATE: DO NOT proceed to Phase 3 without completing this discovery.**

Use Grep to search for existing implementations:
- Search `app/Domain/*` for similar functionality
- Search `docs/architecture.md` for architectural patterns
- Document findings in TodoWrite

**RED FLAG:** Creating new code without discovery → STOP → Go back to Phase 2.
```

### Example 2: Verification

**❌ Bad (vague):**
```markdown
Make sure everything works and tests pass.
```

**✅ Good (explicit):**
```markdown
**NO COMPLETION CLAIMS without running these commands and seeing PASS:**

```bash
# Backend tests
php artisan test

# Frontend tests
npm run test:run
```

**Completion Checklist (ALL must be TRUE):**
- [ ] Test output shows "Tests: X passed, 0 failed"
- [ ] Screenshot saved to `screenshots/`
- [ ] No console errors in screenshot
- [ ] Documentation updated per CIB-003
```

### Example 3: Documentation Integration

**❌ Bad (hardcoded):**
```markdown
Check if BaseApiController exists in app/Http/Controllers/Api/
```

**✅ Good (doc reference):**
```markdown
**FIRST: Read `docs/architecture.md`** to identify:
- Base controller classes (extend these)
- Location of controllers in current structure
```

### Example 4: Tool Usage

**❌ Bad (missing):**
```markdown
Think about the user's request and plan your approach.
```

**✅ Good (explicit):**
```markdown
Use mcp__mcp_docker__sequentialthinking to analyze:
- User's intent and requirements
- Complexity and scope
- Potential approaches
- Risk areas requiring attention
```

---

## Usage Notes

**When to create a command:**
- User requests repeatable workflow
- Workflow has 3+ distinct phases
- Multiple team members need same workflow
- Quality/consistency important

**When NOT to create a command:**
- One-off task
- Simple request (1-2 steps)
- Better suited as a skill (model-invoked)
- Already covered by existing command

**After creating command:**
- User can invoke with `/command-name`
- Shows in `/help` output
- Other team members can use
- Can be improved over time

**Command vs updating existing command:**
- New workflow → New command
- Variation of existing → Update existing
- Complementary → New command
- Replacement → Update existing (or deprecate + new)

---

## Final Statement Template

**When command creation is complete:**

"Command `/[name]` created successfully.

Location: `.claude/commands/[name].md`
Description: [Description from frontmatter]
Phases: [Number] workflow phases
Integration: ✓ TodoWrite, ✓ Sequential thinking, ✓ CIB-003 [if applicable]

Users can now invoke with: `/[name]`

Command follows all established patterns and integrates with project ecosystem."

Remember: Command quality matters. Follow patterns. Integrate with ecosystem. Evidence before claims.
