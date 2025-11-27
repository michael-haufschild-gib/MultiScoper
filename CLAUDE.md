=== CRITICAL INSTRUCTION BLOCK (CIB-001): MANDATORY TOOLS ===

## MANDATORY TOOLS

### For Coding, Research, Analysis, Debugging
```
USE: mcp__mcp_docker__sequentialthinking
WHEN: Coding tasks, research, complex reasoning
WHY: Prevents cognitive overload, ensures systematic approach
```

### For Task Management
```
USE: todo_write
WHEN: Coding tasks, any task with 2+ steps
WHY: Tracks progress, maintains focus
```

### For Task Execution
For each task:

1. **Decide if delegating task an agent is beneficial**
   Delegate to subagent if any of the following applies:
   - Task is complex or multi-step (e.g., write tests, debug a module, generate docs).
   - Task matches a predefined subagent role
   - Task generates large output or context (e.g., scanning 50 files, web research).
   - Task can run independently (no need for constant oversight).
   - Task can be executed in parallel (run multiple subagents at once).

   To delegate a task to an agent do this:
   - Use your agent-selection skill to select the right agent.
   - Call the subagent and provide all the context that they need to do the task at highest quality in the context of the whole codebase.
   - Never send a one-liner prompt to a subagent! Always provide them with all context they need to do the job. Do not let them start from scratch.
   - Always include references to required documentation:
     - `docs/architecture.md`
     - `docs/api.md`
     - `docs/testing.md`
     - include other or more documentation references in the agent prompt if needed for the task
   - **NEVER** just handover the task you have been given to an agent without passing on the work you have already done! Do not waste token on letting agents repeat the research or work you have already done!
   - Be conscious of token usage! Do not duplicate work in the agent that you or another agent have already done!

2. **Testing requirements:**
   - Tests must be meaningful (test actual functionality)
   - Tests must verify correct information, not just rendering

3. **Fix until green:**
   - Run tests after each fix
   - If tests fail, iterate and fix and test again
   - Verify no regressions

=== END CIB-001 ===

=== CRITICAL INSTRUCTION BLOCK (CIB-002): INVESTIGATION-FIRST DEVELOPMENT ===

## FORBIDDEN: Code-First Development

❌ **NEVER** change code based on assumptions
❌ **NEVER** change code without investigating side effects
❌ **NEVER** fix tests without running actual features
❌ **NEVER** declare victory without visual verification
❌ **NEVER** remove code without understanding why it exists

## REQUIRED: Investigation-First Workflow

✅ **ALWAYS** trace code execution path
✅ **ALWAYS** what the purpose of the code is you are about to change
✅ **ALWAYS** analyse impact of a change on whole codebase
✅ **ALWAYS** verify fixes with visual proof

**If you skip investigation, you are creating more bugs and unusable code.STOP AND INVESTIGATE.**

## MANDATORY WORKFLOW
```
1. Analyse and understand user request with sequentialthinking tool
2. Investigate context
3. Investigate impact on other parts of the app
4. Plan implementiation and split into smaller tasks
5. Create tasks with todo_write tool
6. Plan updates to tests and new tests with todo_write tool
7. Implement tasks in todo_write tool in order
8. Implement tests
9. Run tests and fix issues
10. Update documentation
11. Conduct final review
12. Give a short one paragraph summary in chat. NEVER write summary documents. NEVER output long summaries in chat.
```

## ABSOLUTE PROHIBITIONS
- Never skip steps in the workflow to save time or tokens.
- Never simplify or summarize steps in the workflow.
- Never invent things to save time or tokens.

## NO TOKEN OR TIME BUDGET
There is no limit on token or time a task can take. Be thorough. Reason: If you make a mistake now, it will later cost much more time and token to fix it.

=== END CIB-002 ===

## Folder Structure
- Screenshots go into `screenshots/`
- Tests go into `tests/`
- Test Results go into `test-results/`
- Documentation goes into `docs/`
- Do not add files to root that are not specifically required in root location

## Completiong Checklist
- Followed CIB-001 instruction block fully
- Followed CIB-002 instruction block fully
- Tests did not touch the production/development MySQL database
- No summary document written
