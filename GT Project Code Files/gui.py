import customtkinter as ctk
import pandas as pd
from Bipartite import GraphRecommender

# --- Theme Setup ---
ctk.set_appearance_mode("Dark")
ctk.set_default_color_theme("dark-blue")

class MovieRecommenderApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        # Window Config
        self.title("Graph Movie Matchmaker")
        self.geometry("1000x700")
        
        # Load Backend
        self.load_data()

        # Layout Configuration
        self.grid_columnconfigure(1, weight=1)
        self.grid_rowconfigure(0, weight=1)

        # --- LEFT SIDEBAR (Controls) ---
        self.sidebar_frame = ctk.CTkFrame(self, width=220, corner_radius=0)
        self.sidebar_frame.grid(row=0, column=0, sticky="nsew")
        
        self.logo_label = ctk.CTkLabel(
            self.sidebar_frame, 
            text="🎬 CineMatch", 
            font=ctk.CTkFont(size=22, weight="bold")
        )
        self.logo_label.pack(padx=20, pady=(30, 10))

        # Input Area
        self.lbl_id = ctk.CTkLabel(self.sidebar_frame, text=f"User ID ({self.min_uid}-{self.max_uid}):")
        self.lbl_id.pack(padx=20, pady=(20, 0), anchor="w")

        self.entry_uid = ctk.CTkEntry(self.sidebar_frame, placeholder_text="e.g. 1")
        self.entry_uid.pack(padx=20, pady=5)

        self.btn_run = ctk.CTkButton(
            self.sidebar_frame, 
            text="Find Movies", 
            fg_color="transparent", 
            border_width=2,
            text_color=("gray10", "#DCE4EE"),
            command=self.run_recommendation
        )
        self.btn_run.pack(padx=20, pady=20)

        # Status Label
        self.status_label = ctk.CTkLabel(self.sidebar_frame, text="", text_color="yellow")
        self.status_label.pack(padx=20, pady=10)

        # --- RIGHT MAIN AREA (Results) ---
        self.main_frame = ctk.CTkFrame(self, fg_color="transparent")
        self.main_frame.grid(row=0, column=1, sticky="nsew", padx=20, pady=20)

        # Title
        self.lbl_result_title = ctk.CTkLabel(
            self.main_frame, 
            text="Recommendations", 
            font=ctk.CTkFont(size=24, weight="bold")
        )
        self.lbl_result_title.pack(anchor="w", pady=(0, 10))

        # --- HEADER ROW (Static) ---
        # We create a specific frame just for headers to align with the cards
        self.header_frame = ctk.CTkFrame(self.main_frame, fg_color="transparent", height=30)
        self.header_frame.pack(fill="x", padx=5)
        
        # Using exact widths or weights to match the row layout
        # 1. Match % (Left)
        ctk.CTkLabel(self.header_frame, text="Match %", width=60, font=("Arial", 12, "bold"), anchor="w").pack(side="left", padx=(15, 10))
        # 2. Title (Left)
        ctk.CTkLabel(self.header_frame, text="Movie Title", font=("Arial", 12, "bold"), anchor="w").pack(side="left", padx=10)
        # 3. Rating (Right)
        ctk.CTkLabel(self.header_frame, text="Rating", width=80, font=("Arial", 12, "bold"), anchor="e").pack(side="right", padx=(5, 15))
        # 4. Score (Right)
        ctk.CTkLabel(self.header_frame, text="Graph Score", width=80, font=("Arial", 12, "bold"), anchor="e").pack(side="right", padx=5)

        # Scrollable Container
        self.scroll_frame = ctk.CTkScrollableFrame(self.main_frame, label_text="")
        self.scroll_frame.pack(fill="both", expand=True)

    def load_data(self):
        print("Initializing system...")
        movies = pd.read_csv('data/movies.csv')
        ratings = pd.read_csv('data/ratings.csv')
        
        self.titles = movies.set_index('movieId')['title'].to_dict()
        self.valid_users = set(ratings['userId'].unique())
        self.min_uid = ratings['userId'].min()
        self.max_uid = ratings['userId'].max()
        
        self.rec = GraphRecommender(ratings, min_rating=3.5)
        print("System Ready.")

    def get_color_for_strength(self, label):
        if label == "Top Pick": return "#2CC985"  # Green
        if label == "Strong":   return "#3B8ED0"  # Blue
        if label == "Good":     return "#E1AD01"  # Yellow
        return "#828282"                          # Gray

    def run_recommendation(self):
        # 1. Clear previous results
        for widget in self.scroll_frame.winfo_children():
            widget.destroy()
        
        self.status_label.configure(text="Processing...", text_color="white")
        self.update()

        # 2. Validation
        user_input = self.entry_uid.get()
        if not user_input.isdigit():
            self.status_label.configure(text="❌ Invalid ID", text_color="red")
            return
        
        uid = int(user_input)
        if uid not in self.valid_users:
            self.status_label.configure(text="❌ Not Found", text_color="red")
            return

        # 3. Get Logic
        results = self.rec.recommend(uid)
        
        if not results:
            self.status_label.configure(text="No matches.", text_color="orange")
            lbl = ctk.CTkLabel(self.scroll_frame, text="No recommendations found for this user.")
            lbl.pack(pady=20)
            return

        self.status_label.configure(text="Done!", text_color="#2CC985")
        top_score = results[0][1]

        # 4. Create "Cards" for each movie
        for mid, score in results:
            title = self.titles.get(mid, f"Unknown ({mid})")
            pct = (score / top_score) * 100
            
            # Logic for strength label
            if pct >= 90: label_text = "Top Pick"
            elif pct >= 75: label_text = "Strong"
            elif pct >= 50: label_text = "Good"
            else: label_text = "Fair"
            
            color = self.get_color_for_strength(label_text)

            # --- THE CARD ---
            card = ctk.CTkFrame(self.scroll_frame, border_color="gray30", border_width=1)
            card.pack(fill="x", pady=5, padx=5)

            # 1. Match Percentage Badge (Left)
            badge = ctk.CTkButton(
                card, 
                text=f"{int(pct)}%", 
                width=50, 
                fg_color=color, 
                hover=False
            )
            badge.pack(side="left", padx=10, pady=10)

            # 2. Title Label (Middle - Expands)
            lbl_title = ctk.CTkLabel(
                card, 
                text=title, 
                font=("Arial", 14), 
                anchor="w"
            )
            lbl_title.pack(side="left", padx=10, fill="x", expand=True)

            # 3. Strength Label (Right)
            lbl_strength = ctk.CTkLabel(
                card, 
                text=label_text, 
                width=80,
                text_color=color,
                font=("Arial", 11, "bold"),
                anchor="e"
            )
            lbl_strength.pack(side="right", padx=(5, 15))

            # 4. Raw Score (Right, before Strength)
            lbl_score = ctk.CTkLabel(
                card,
                text=f"{score:.3f}",
                width=80,
                font=("Consolas", 12),
                text_color="gray70",
                anchor="e"
            )
            lbl_score.pack(side="right", padx=5)

        # 5. Show History (User's Watched Movies)
        watched_ids = self.rec.get_user_movies(uid)
        
        if watched_ids:
            # Separator
            ctk.CTkFrame(self.scroll_frame, height=2, fg_color="gray30").pack(fill="x", pady=(30, 10), padx=10)
            
            # Label
            header_hist = ctk.CTkLabel(
                self.scroll_frame, 
                text=f"History: User {uid} has already watched {len(watched_ids)} movies (Excluded)",
                font=("Arial", 12, "bold"),
                text_color="gray60"
            )
            header_hist.pack(anchor="w", padx=10)

            # Get first 10 titles
            watched_titles = []
            for m in list(watched_ids)[:10]:
                watched_titles.append(self.titles.get(m, str(m)))
            
            history_text = ", ".join(watched_titles)
            if len(watched_ids) > 10:
                history_text += ", ..."

            # Paragraph block for titles
            lbl_hist_list = ctk.CTkLabel(
                self.scroll_frame,
                text=history_text,
                font=("Arial", 11),
                text_color="gray50",
                wraplength=600,  # Wrap text if it gets too long
                justify="left",
                anchor="w"
            )
            lbl_hist_list.pack(anchor="w", padx=10, pady=(0, 20))

if __name__ == "__main__":
    app = MovieRecommenderApp()
    app.mainloop()