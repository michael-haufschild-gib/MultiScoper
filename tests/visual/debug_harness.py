import time
from harness_client import HarnessClient

client = HarnessClient()

print("Checking health...")
print(client.get("/health"))

print("Resetting state...")
client.reset_state()

print("Adding oscillator...")
client.add_oscillator(name="Osc 1")

print("Showing editor...")
client.show_editor(0)

print("Waiting 5 seconds...")
time.sleep(5)

print("Setting audio generator...")
client.set_audio_generator(0, waveform="sine")

print("Analyzing internal...")
try:
    res = client.get("/analyze/internal")
    print("Analysis result:", res)
except Exception as e:
    print("Analysis failed:", e)

print("Done.")
