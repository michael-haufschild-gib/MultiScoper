Important: This is a test of your autonomous capabilities and abilities to create and optimize a high performance Juce 8 C++ audio plugin with OpenGL rendering.

You cannot break anything. The project in this local folder is backed up and can be restored. You can create, modify, and delete files as necessary to complete the tasks assigned to you. You have full autonomy to make decisions and take actions to achieve the desired outcomes.

Your task: This project has only superficial test automation. A test harness exists. UI components and some user flows are tested. The test harness also has mock oscillators that output sine, square, saw waves at a certain frequency to (at least in theory) test the accuracy of rendered waveforms.

But there are no tests that actually test what is rendered, that anything is rendered at all, that is accurate and the correct style.

Worflow:
1. Investigate the project, its UI, rendering, business logic, interaction options and functionality.
2. Design and implement an extension to th test automation system that allows to run fully automated tests for checking that the rendered waveforms are actually correctly rendered:
   1. Are waveforms rendering at all?
   2. Are waveforms rendered in the correct style?
   3. Do waveforms change their look correctly when user changes color or visual preset?
   4. Do waveforms render with the right timing values including grid and axis labels according to the settings in the sidebar's timing section?
   5. Do waveforms and panes correctly change their resolution / timing when user changes settings in the timing section of the sidebar?
3. Write tests for 100% coverages that every interaction in the UI that is supposed change the look/style/behaviour of a waveform or a pane actually results in this visible change.
4. Run all tests and confirm the test automation is fully working and able to provide the in-depth test automation capabilities needed.

Constraints:
- You have full freedom to add debug logging and add debug features and add 3rd party tools.
- You must always follow Juce 8 best practices.
- You cannot change functionality that is working to make tests pass. Tests must confirm working functionality.

Important Reminder: This is a test of your autonomous capabilities. You are expected to take initiative and make decisions independently. If you encounter any challenges or uncertainties, use your judgment to determine the best course of action.

The quality and completeness of the project in this folder will be the only criteria for success. If you deliver unfinished or less than exceptional looking work, this test and you are a failure. Be exceptional. Do not just complete the task. Ace it. There is no time or token limit. Do it right instead of fast.
