---
description: Create agent-friendly documentation (patterns, decisions, templates) not human reference catalogs
---

# Bootstrap Agent Context Documentation

**Purpose**: Create 5 documentation files that serve as **context for LLM coding agents**, teaching HOW to make decisions and implement features (patterns, templates, decision trees), NOT cataloging WHAT exists (exhaustive listings).

**Documentation Philosophy**:
- Docs = Agent context/system prompts
- Teach patterns, not inventory
- Provide copy-paste templates
- Include decision trees
- Token-efficient (no exhaustive listings)

**What this creates**:
- `docs/architecture.md` - WHERE to put code, WHAT patterns to follow
- `docs/api.md` - HOW to create endpoints (step-by-step with templates)
- `docs/database.md` - HOW to work with Eloquent (patterns, not schema dumps)
- `docs/testing.md` - HOW to write tests (patterns, not test inventory)
- `docs/deployment.md` - HOW to run locally (commands, not production infra)

**What this does NOT create**:
- ❌ Exhaustive file/directory listings (changes too often)
- ❌ Complete database schema dumps (use Eloquent patterns instead)
- ❌ Full API endpoint reference (use HOW-TO patterns instead)
- ❌ Comprehensive test inventories (use testing patterns instead)
- ❌ Human-oriented verbose documentation

---

## Phase 0: Plan Documentation Approach

Use TodoWrite to track:
- [ ] Analyze codebase for PATTERNS (not inventory)
- [ ] Create docs/architecture.md (patterns + decision trees)
- [ ] Create docs/api.md (HOW-TO with templates)
- [ ] Create docs/database.md (Eloquent patterns)
- [ ] Create docs/testing.md (testing patterns)
- [ ] Create docs/deployment.md (local dev guide)
- [ ] Verify: no exhaustive listings, templates present, token-efficient

---

## Phase 1: Analyze Codebase for PATTERNS (Not Inventory)

**CRITICAL**: Focus on discovering PATTERNS that help agents make decisions, NOT creating exhaustive inventories.

### What to Discover (Patterns & Conventions)

**Backend Patterns**:
- Architectural pattern: DDD? Traditional Laravel? Modular?
- Domain structure: How are domains organized? Naming pattern?
- Base classes: What should agents extend? (BaseController, BaseRequest, BasePolicy)
- Naming conventions: Controllers, models, requests, resources, policies
- Common patterns: Translations? Polymorphic relations? Soft deletes?

**Frontend Patterns**:
- Framework & version: Vue 3? React? TypeScript?
- Component organization: By domain? By type? Naming pattern?
- State management: Pinia? Composables?
- API integration: Which composable? Pattern for REST calls?

**Database Patterns**:
- ORM: Always Eloquent? Raw SQL when?
- Conventions: Table naming, foreign keys, timestamps, soft deletes
- Relationship patterns: HasMany, BelongsToMany, Polymorphic
- Translation pattern: Is there a {Model}Translation pattern?

**Testing Patterns**:
- Backend: PHPUnit version, test database (SQLite?), factories
- Frontend: Vitest? Playwright? Test organization?
- Safety measures: How is production DB protected?

**Development Patterns**:
- Local dev: Laravel Sail? Docker? Commands to run?
- Build process: Vite? Webpack? Dev vs production?
- Key commands: How to run tests, migrations, build frontend?

### What NOT to Document (Changes Too Often)

**Skip these (token waste, maintenance burden)**:
- ❌ Every file in every directory
- ❌ Every database table with full schema
- ❌ Every API endpoint with full docs
- ❌ Every test file
- ❌ Current state inventories

**Instead document**:
- ✅ Folder PURPOSE patterns
- ✅ Database conventions and relationship patterns
- ✅ HOW to create endpoints (not which ones exist)
- ✅ HOW to write tests (not which exist)
- ✅ Current state PATTERNS

### Discovery Commands

```bash
# Framework versions
grep "laravel/framework" composer.json
grep "vue" package.json

# Find base classes
find app -name "Base*.php"

# Identify patterns
ls -la app/Domain/*/Models/
ls -la app/Domain/*/Http/Controllers/Api/

# Database conventions (observe from migrations)
ls -la database/migrations/ | head -10

# Testing setup
cat phpunit.xml | grep -A 2 "<env"
cat vitest.config.ts | grep -A 5 "export default"
```

**Deliverable**: Understanding of architectural patterns, base classes, naming conventions, common patterns. NO exhaustive listings.

---

## Phase 2: Create docs/architecture.md

**Purpose**: Help agents decide WHERE to put code and WHAT patterns to follow.

**Structure**:
```markdown
# Architecture Guide for LLM Coding Agents

**Purpose**: This document helps you make architectural decisions and locate/place code correctly.

**Tech Stack**: [Brief: Laravel X + Vue X + DB + Cache + Queue]

---

## Core Architectural Principles

### 1. [Architecture Pattern Name]
**Mental Model**: [One sentence explaining the pattern]
**Decision Rule**: [How to make choices using this pattern]

### 2. Where to Put New Code
```
app/Domain/{DomainName}/
├── Http/Controllers/Api/  # REST controllers
├── Models/                # Eloquent models
├── Policies/              # Authorization
└── ...
```

**When creating new functionality in domain X**:
1. Model → `app/Domain/X/Models/`
2. API Controller → `app/Domain/X/Http/Controllers/Api/`
[etc.]

### 3. Controller Patterns
**Two Types**:
- REST API Controllers: Extend `BaseApiController`, return JSON
- View Controllers: Render Blade + Vue, no business logic

[Include code template for each]

### 4. [Other Key Patterns with Templates]

## Quick Decision Tree

**"Where do I put X?"**
1. New domain model? → `app/Domain/{Domain}/Models/`
2. REST API endpoint? → `app/Domain/{Domain}/Http/Controllers/Api/`
[etc.]

**"How do I add CRUD feature?"**
1. Create model in domain folder
2. Create migration
[10-step checklist with patterns]

## Common Mistakes

❌ **Don't**: [Anti-pattern]
✅ **Do**: [Correct pattern]
```

**Key Sections** (Required):
1. **Core Architectural Principles** - DDD, folder structure pattern, base classes
2. **Where to Put New Code** - Decision tree with folder patterns
3. **Controller Patterns** - Two types with templates
4. **Frontend Architecture** - Component organization pattern
5. **Routing Conventions** - How to register routes
6. **Authentication & Authorization** - Sanctum + Policies pattern
7. **Database Patterns** - Eloquent conventions
8. **Testing Conventions** - Where tests go, safety rules
9. **Queue & Jobs** - Pattern for background jobs
10. **Common Mistakes** - Anti-patterns with corrections
11. **Key Base Classes** - What to extend
12. **File Naming Conventions** - Patterns for consistency
13. **Quick Decision Tree** - "Where do I put X?"

**What to EXCLUDE**:
- ❌ Full directory tree with all files
- ❌ List of all models
- ❌ List of all controllers
- ❌ Comprehensive package inventory

**Backup existing if present**:
```bash
if [ -f docs/architecture.md ]; then
    cp docs/architecture.md docs/architecture.md.backup-$(date +%Y%m%d-%H%M%S)
fi
```

---

## Phase 3: Create docs/api.md

**Purpose**: Teach agents HOW to create and modify API endpoints.

**Structure**:
```markdown
# API Development Guide for LLM Coding Agents

**Purpose**: This document teaches you how to create, modify, and maintain RESTful API endpoints.

**Base URL**: `/api/v1`

---

## API Architecture Pattern

### REST Resources (Standard CRUD)
**Pattern**: Every model gets standard REST routes
[Show route structure]

### Authentication
**Two Methods**: Session-based (admin), Token-based (API)
[Show middleware patterns]

---

## How to Create a New API Endpoint

### Step 1: Create the Controller
**Location**: `app/Domain/{Domain}/Http/Controllers/Api/`
**Template**:
```php
[Full copy-paste template]
```
**Key Points**: [What to know]

### Step 2: Create Form Requests
[Template for Store and Update]

### Step 3: Create API Resource
[Template for transformation]

### Step 4: Create Policy
[Template for authorization]

### Step 5: Register Routes
[Template for route registration]

---

## Response Format Standards
[JSON examples with ApiResponse helper]

## Common Patterns
1. Eager Loading (Prevent N+1) [with examples]
2. Query Filtering [pattern]
3. Bulk Operations [pattern]
4. Custom Actions [pattern]
5. File Uploads [pattern]

## Authorization Patterns
[3 patterns with examples]

## Error Handling
[Patterns for validation, 404, 403, 500]

## Testing Your API
[Feature test template]

## Quick Checklist for New Endpoint
- [ ] Controller extends BaseApiController
- [ ] Form Requests created
[etc.]

## Common Mistakes
❌ **Don't**: Return raw Eloquent models
✅ **Do**: Use API Resources
```

**What to EXCLUDE**:
- ❌ List of all endpoints (duplicates route:list)
- ❌ Full API reference documentation
- ❌ Endpoint-by-endpoint documentation

---

## Phase 4: Create docs/database.md

**Purpose**: Teach agents HOW to work with database using Eloquent ORM.

**Structure**:
```markdown
# Database Development Guide for LLM Coding Agents

**Purpose**: This document teaches you how to work with the database using Eloquent ORM.

**Database**: MySQL (production), SQLite in-memory (testing)

---

## Core Principles

### Eloquent-First
**Rule**: Use Eloquent ORM for all database operations. Rarely write raw SQL.
**Why**: Security, readability, relationships, events, soft deletes

---

## Creating a New Model

### Step 1: Generate Model + Migration
```bash
php artisan make:model Domain/{Domain}/Models/YourModel -m
```

### Step 2: Define Model
**Location**: `app/Domain/{Domain}/Models/YourModel.php`
**Template**:
```php
[Full copy-paste template with fillable, casts, relationships, scopes]
```

### Step 3: Create Migration
**Template**:
```php
[Full migration template with conventions]
```

### Step 4: Run Migration
[Command]

### Step 5: Create Factory
**Template**:
```php
[Factory template with states]
```

---

## Relationship Patterns

### 1. One-to-Many (hasMany / belongsTo)
[Template with migration and usage]

### 2. Many-to-Many (belongsToMany)
[Template with pivot table and usage]

### 3. Polymorphic Relations
[Template and usage]

### 4. Translation Pattern
[Pattern used in codebase with example]

---

## Query Patterns

### 1. Preventing N+1 Queries
[Bad vs Good example]

### 2. Query Scopes
[Define and use pattern]

### 3. Filtering and Sorting
[Pattern]

### 4. Aggregates
[Examples]

### 5. Soft Deletes
[Pattern and usage]

---

## Migration Patterns
[Adding columns, renaming, foreign keys, dropping]

## Database Conventions (Critical)
[Naming, required columns, indexes]

## Factory Patterns
[Basic, with relationships, states]

## Common Mistakes
❌ **Don't**: Write raw SQL queries
✅ **Do**: Use Eloquent or Query Builder
[etc.]

## Testing Database (Critical Safety)
**Rule**: Tests NEVER touch MySQL. Always SQLite in-memory.
[Reference CLAUDE.md safeguards]

## Quick Reference
[Common artisan commands]
```

**What to EXCLUDE**:
- ❌ Full schema dump of all tables
- ❌ List of all columns for every table
- ❌ Comprehensive relationship diagram
- ❌ Every migration file content

---

## Phase 5: Create docs/testing.md

**Purpose**: Teach agents HOW to write and run tests.

**Structure**:
```markdown
# Testing Guide for LLM Coding Agents

**Purpose**: This document teaches you how to write and run tests in this CMS.

**Tech Stack**: PHPUnit 11 (backend), Vitest 4 (frontend), Playwright (E2E)

---

## Testing Principles

### 1. Test Database Safety (CRITICAL)
**Rule**: Tests NEVER touch MySQL. Always SQLite in-memory.
[Explain 4-layer defense, reference CLAUDE.md]

### 2. TDD Workflow
[When to write tests, pattern]

### 3. Test Coverage Targets
[Percentages and priorities]

---

## Backend Testing (PHPUnit)

### How to Write Feature Tests
**Template**:
```php
[Full copy-paste feature test template]
```

**Patterns**:
- Authentication testing [pattern]
- API testing [pattern]
- Database testing [pattern]

### How to Write Unit Tests
[Template and pattern]

### Using Factories
[Pattern and examples]

---

## Frontend Testing (Vitest)

### How to Write Composable Tests
[Template]

### How to Write Component Tests
[Template]

**Patterns**:
[Key patterns]

---

## E2E Testing (Playwright)

### How to Write E2E Tests
**Template**: Use `.claude/commands/meta/e2e-template.ts`
[Key requirements and patterns]

---

## Running Tests

### Backend Tests
```bash
./vendor/bin/sail test
[Other commands]
```

### Frontend Tests
```bash
npm run test:run
[Other commands]
```

### Full Test Suite
[Commands for everything]

---

## Common Testing Patterns
[5-10 patterns with examples]

## Debugging Tests
[Strategies]

## Common Mistakes
❌ **Don't**: Tests touch production database
✅ **Do**: Always use SQLite in-memory
[etc.]

## Quick Reference
[Common commands table]
```

**What to EXCLUDE**:
- ❌ List of all test files
- ❌ Comprehensive test coverage report
- ❌ Every test pattern in exhaustive detail

---

## Phase 6: Create docs/deployment.md

**Purpose**: Teach agents HOW to run the app locally and key commands.

**Structure**:
```markdown
# Local Development Guide for LLM Coding Agents

**Purpose**: This document teaches you how to run the application locally and execute key commands.

**Tech Stack**: Laravel Sail (Docker), Vite, MySQL, Redis

---

## Local Development Setup

### Step 1: Prerequisites
[List requirements]

### Step 2: Clone and Install
```bash
[Step-by-step commands]
```

### Step 3: Configuration
[.env setup, key generation]

### Step 4: Database Setup
```bash
[Migration commands]
```

### Step 5: Start Services
```bash
./vendor/bin/sail up -d
```

---

## Running the Application

### Starting Development Server
```bash
./vendor/bin/sail up -d  # Start Docker containers
npm run dev              # Start Vite dev server
```

### Stopping Services
[Commands]

### Restarting Services
[Commands]

---

## Running Tests

### Backend Tests
```bash
./vendor/bin/sail test
```

### Frontend Tests
```bash
npm run test:run
```

### E2E Tests
```bash
npm run test:e2e
```

---

## Building Frontend Assets

### Development Build
```bash
npm run dev
```

### Production Build
```bash
npm run build
```

---

## Common Development Commands

### Database Commands
```bash
# Run migrations
./vendor/bin/sail artisan migrate

# Seed database
./vendor/bin/sail artisan db:seed

# Rollback
./vendor/bin/sail artisan migrate:rollback
```

### Creating New Files
```bash
# Model + migration
./vendor/bin/sail artisan make:model Domain/X/Models/YourModel -m

# Controller
./vendor/bin/sail artisan make:controller Domain/X/Http/Controllers/Api/YourController

# Factory
./vendor/bin/sail artisan make:factory YourModelFactory
```

### Cache Commands
```bash
# Clear all caches
./vendor/bin/sail artisan cache:clear
./vendor/bin/sail artisan config:clear
./vendor/bin/sail artisan route:clear
./vendor/bin/sail artisan view:clear

# Optimize (production)
./vendor/bin/sail artisan optimize
```

### Queue Commands
```bash
# Start Horizon
./vendor/bin/sail artisan horizon

# Check queue status
./vendor/bin/sail artisan queue:work
```

### Debugging Commands
```bash
# Tinker REPL
./vendor/bin/sail artisan tinker

# View logs
./vendor/bin/sail logs -f

# Run specific artisan command
./vendor/bin/sail artisan [command]
```

---

## Troubleshooting

### Docker Container Issues
**Issue**: Containers won't start
**Solution**: [Steps]

### Laravel Issues
**Issue**: 500 errors
**Solution**: [Steps]

### Frontend Issues
**Issue**: Vite won't start
**Solution**: [Steps]

### Test Issues
**Issue**: Tests touching MySQL
**Solution**: [Reference CLAUDE.md safeguards]

---

## Quick Cheatsheet

| Task | Command |
|------|---------|
| Start dev | `./vendor/bin/sail up -d && npm run dev` |
| Run tests | `./vendor/bin/sail test && npm run test:run` |
| Migrate DB | `./vendor/bin/sail artisan migrate` |
| Build frontend | `npm run build` |
| Clear caches | `./vendor/bin/sail artisan optimize:clear` |
```

**What to EXCLUDE**:
- ❌ Production deployment procedures
- ❌ Server configuration details
- ❌ CI/CD pipeline documentation
- ❌ Complex DevOps infrastructure

---

## Phase 7: Verification

**MANDATORY: Verify all docs before claiming complete.**

### Structural Verification
```bash
# Check all files exist
ls -la docs/*.md

# Count lines (architecture.md should be substantial but not exhaustive)
wc -l docs/*.md

# Verify no TODOs or placeholders
grep -r "\[TODO\]" docs/
grep -r "\[PLACEHOLDER\]" docs/
grep -r "\[\.\.\.\]" docs/ | grep -v "example"  # Exclude JSON examples
```

### Content Quality Checks

**For EACH documentation file, verify**:
- [ ] Starts with "Guide for LLM Coding Agents" + Purpose + Tech Stack
- [ ] Organized by patterns (not components)
- [ ] Every pattern has copy-paste template
- [ ] Includes decision trees ("Where do I put X?")
- [ ] Ends with "Common Mistakes" section
- [ ] Uses directive language ("Do", "Don't", "Rule", "Always")
- [ ] NO exhaustive listings (files, tables, endpoints)
- [ ] Self-contained (no cross-references to other docs)

### Token Efficiency Check

**Verify EXCLUDED** (these waste tokens):
- [ ] No full directory tree with all files
- [ ] No comprehensive database schema dump
- [ ] No list of all API endpoints
- [ ] No list of all test files
- [ ] No verbose prose explanations

**Verify INCLUDED** (these provide value):
- [ ] Folder PURPOSE patterns
- [ ] Copy-paste code templates
- [ ] Decision trees
- [ ] Naming conventions
- [ ] Base classes to extend
- [ ] Common mistakes

### Cross-Reference Check
- [ ] Tech stack versions consistent across all docs
- [ ] File paths use correct patterns
- [ ] Commands are accurate for this project (Sail vs native)

---

## Final Output

**After ALL verifications pass:**

```
Documentation bootstrap complete. Created:
- docs/architecture.md ([X] lines) - Decision patterns, base classes, folder structure
- docs/api.md ([X] lines) - Endpoint creation HOW-TO with templates
- docs/database.md ([X] lines) - Eloquent patterns and conventions
- docs/testing.md ([X] lines) - Test writing patterns
- docs/deployment.md ([X] lines) - Local dev guide and commands

Verified:
✓ No exhaustive listings (files, tables, endpoints)
✓ Every pattern has copy-paste template
✓ Decision trees present in each doc
✓ Common mistakes sections included
✓ Token-efficient (pattern-focused, not inventory)
✓ Agent-optimized (can implement CRUD with just these docs)

All documentation reflects codebase patterns as of [date].
Ready for use as agent context.
```

**FORBIDDEN** (do NOT create):
- ❌ `docs/DOCUMENTATION_SUMMARY.md`
- ❌ `docs/CODEBASE_ANALYSIS_REPORT.md`
- ❌ Summary or analysis documents

---

## Usage Notes

**When to run**: First time setup, after major refactoring, when docs severely outdated

**Maintenance**: Prefer incremental updates. Only re-bootstrap when patterns change significantly.

**Philosophy**: These docs teach agents HOW to work with your codebase patterns, not WHAT your codebase currently contains. They should rarely need updates because patterns are more stable than inventory.
