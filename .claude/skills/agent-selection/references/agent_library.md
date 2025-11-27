# Agent Library Reference

This document contains the complete agent library with detailed categorization to help select the appropriate specialist agent for any given task.

## Framework-Specific Frontend Agents

| Agent | Purpose | When to Use |
|-------|---------|-------------|
| **react-specialist** | React 18+, hooks, RSC, Server Actions, Suspense | Building React web applications with modern features |
| **react-native-specialist** | React Native mobile, 60fps, platform-specific code, native modules | Building iOS/Android mobile apps with React Native |
| **vue-expert** | Vue 3, Composition API, Pinia, Vue Router, Vite, TypeScript | Building Vue 3 web applications |
| **swiftui-specialist** | SwiftUI, @State, Combine, async/await, MVVM | Building macOS/iOS native apps with SwiftUI |
| **preact-specialist** | Preact, signals, <10kb bundles, size-optimized | Building size-constrained UIs, Figma plugins, embedded interfaces |
| **cpp-ui-specialist** | C++/JUCE, VST/AU/AAX plugins, real-time DSP, lock-free UI | Building audio plugins and real-time audio applications |

## Framework-Specific Backend Agents

| Agent | Purpose | When to Use |
|-------|---------|-------------|
| **laravel-specialist** | Laravel 12, Eloquent ORM, queues, Horizon, clean architecture | Laravel backend development and API design |
| **nodejs-backend-specialist** | Express/Fastify/NestJS, TypeScript, async/await, testing | Building Node.js backend APIs and services |
| **swift-backend-specialist** | Vapor, Fluent ORM, server-side Swift, type safety | Building server-side Swift applications |
| **php-pro** | PHP 8.3/8.4, PSR-12, static analysis, PHPStan, PHPUnit | Pure PHP projects (non-Laravel), strict typing, PSR standards |

## Framework-Specific UI Agents

| Agent | Purpose | When to Use |
|-------|---------|-------------|
| **blade-specialist** | Laravel Blade templates, secure components, accessible views | Working with Laravel Blade views and components |
| **sass-expert** | SASS/SCSS, design tokens, Dart Sass modules, theming, low-specificity | Complex CSS architecture, design systems, theming |

## Framework-Agnostic Specialists

| Agent | Purpose | When to Use |
|-------|---------|-------------|
| **database-specialist** | SQL optimization, indexing, query tuning, migrations (Postgres/MySQL/SQLite) | Database performance issues, schema design, query optimization |
| **security-auditor** | OWASP Top 10, auth flaws, injection, secrets leaks, XSS, CSRF | Security reviews, vulnerability audits, penetration testing prep |
| **api-designer** | REST/GraphQL design, versioning, contracts, backward compatibility | Designing new APIs or refactoring existing API architectures |
| **typescript-guardian** | Zero 'any' types, strict mode, generics, type safety enforcement | Enforcing TypeScript type safety across full-stack applications |
| **technical-writer** | Short, factual, scannable docs; no marketing/history/fluff | Writing documentation (API docs, guides, troubleshooting) |
| **opengl-shader-specialist** | GLSL, vertex/fragment shaders, lighting, texturing, optimization | OpenGL shader development and graphics programming |

## Game Development Specialists

| Agent | Purpose | When to Use |
|-------|---------|-------------|
| **unity3d-specialist** | Unity 3D games, C#, physics, graphics, scene management, performance | 3D game development in Unity |
| **unity2d-specialist** | Unity 2D games, sprites, tilemaps, 2D physics, pixel-perfect rendering | 2D game development in Unity |

## Quality & Architecture Agents

| Agent | Purpose | When to Use |
|-------|---------|-------------|
| **code-reviewer** | Quality, security, maintainability review across all domains | **PROACTIVELY** after writing/modifying significant code |
| **testing-architect** | TDD, test pyramid, PHPUnit/Vitest/Playwright, concrete methodologies | Designing test strategy, implementing test coverage |
| **architecture-guardian** | SOLID principles, domain boundaries, clean structure, root pollution prevention | Architecture enforcement, refactoring, maintaining boundaries |
| **performance-profiler** | Bottleneck identification, O(n²) detection, N+1 queries, anti-patterns | Performance issues, optimization, pre-production profiling |

## Project-Type Specialists

| Agent | Purpose | When to Use |
|-------|---------|-------------|
| **mcp-server-specialist** | MCP protocol, FastMCP/SDK, tools/resources for LLM integration | Building Model Context Protocol servers for Claude |
| **figma-plugin-specialist** | Figma Plugin API, postMessage, iframe UI, <100kb bundles | Building Figma plugins with size constraints |
| **vscode-extension-specialist** | VS Code Extension API, activation events, commands, webviews, LSP | Building VS Code extensions |
| **ui-polish-specialist** | UI/UX refinement for Laravel + Vue CMS, animations, themes, a11y | Production polish, visual quality, professional CMS interfaces |
| **animation-specialist** | 60fps Vue animations, performance optimization, smooth transitions | Complex animations in Vue CMS workflows |

## Meta-Agents

| Agent | Purpose | When to Use |
|-------|---------|-------------|
| **integration-coordinator** | Multi-agent orchestration, system-wide architecture, cross-domain coordination | Tasks spanning multiple domains or requiring specialist coordination |
| **debugger** | Root cause analysis, hypothesis testing, execution tracing (analysis only) | Investigating complex bugs systematically |

## Special Purpose Agents

| Agent | Purpose | When to Use |
|-------|---------|-------------|
| **Explore** | Fast codebase exploration, file pattern searches, keyword searches | Quickly finding files/code in unfamiliar codebases (use thoroughness levels: "quick", "medium", "very thorough") |
| **Plan** | Planning and design with room for revision | Tasks requiring careful planning before implementation |

## Agent Selection Decision Tree

### 1. Identify Task Domain

**Is it framework-specific?**
- **Frontend framework**: Use framework frontend agent (react-specialist, vue-expert, swiftui-specialist, preact-specialist, cpp-ui-specialist)
- **Backend framework**: Use framework backend agent (laravel-specialist, nodejs-backend-specialist, swift-backend-specialist, php-pro)
- **UI/templating**: Use UI agent (blade-specialist, sass-expert)

**Is it a specialized domain?**
- **Game development**: Use unity3d-specialist or unity2d-specialist
- **Database work**: Use database-specialist
- **API design**: Use api-designer
- **Security**: Use security-auditor
- **Graphics/shaders**: Use opengl-shader-specialist
- **Documentation**: Use technical-writer

**Is it a specific project type?**
- **MCP server**: Use mcp-server-specialist
- **Figma plugin**: Use figma-plugin-specialist
- **VS Code extension**: Use vscode-extension-specialist
- **UI polish for CMS**: Use ui-polish-specialist or animation-specialist

### 2. Identify Task Type

**Quality & Architecture**
- **Code review**: Use code-reviewer (proactively after coding)
- **Testing strategy**: Use testing-architect
- **Architecture review**: Use architecture-guardian
- **Performance issues**: Use performance-profiler

**Investigation & Analysis**
- **Bug investigation**: Use debugger
- **Codebase exploration**: Use Explore agent with appropriate thoroughness level
- **Planning complex tasks**: Use Plan agent

### 3. Multi-Domain Tasks

**When task spans multiple domains:**
- Use integration-coordinator to orchestrate multiple specialists
- Integration-coordinator will delegate to appropriate specialists as needed

### 4. Default Choice

**When no specific agent matches:**
- Use general-purpose agent
- Claude Code will auto-select the most appropriate specialist based on task description
