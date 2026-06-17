import tkinter as tk
from collections import Counter

class KeyboardVisualizer:
    def __init__(self, root):
        self.root = root
        self.root.title("Keyboard Event Visualizer")

        self.key_counts = Counter()

        self.info_label = tk.Label(
            root,
            text="Click inside this window and start typing.",
            font=("Arial", 12)
        )
        self.info_label.pack(pady=10)

        self.stats_label = tk.Label(
            root,
            text="No keys pressed yet.",
            font=("Arial", 10)
        )
        self.stats_label.pack(pady=5)

        self.log_box = tk.Text(root, height=15, width=60)
        self.log_box.pack(padx=10, pady=10)

        root.bind("<KeyPress>", self.on_key_press)

    def on_key_press(self, event):
        key = event.keysym

        self.key_counts[key] += 1

        self.log_box.insert(
            tk.END,
            f"Key Pressed: {key}\n"
        )
        self.log_box.see(tk.END)

        total = sum(self.key_counts.values())
        most_common = self.key_counts.most_common(5)

        stats_text = (
            f"Total Key Presses: {total}\n"
            f"Top Keys: {most_common}"
        )

        self.stats_label.config(text=stats_text)

if __name__ == "__main__":
    root = tk.Tk()
    app = KeyboardVisualizer(root)
    root.geometry("600x400")
    root.mainloop()
