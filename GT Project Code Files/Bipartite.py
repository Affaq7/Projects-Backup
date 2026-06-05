import networkx as nx
import pandas as pd
from collections import defaultdict

class GraphRecommender:
    def __init__(self, df, min_rating=4.0):
        self.df = df
        self.min_rating = min_rating
        self.G = nx.Graph()
        self.build_graph()

    def _get_u_node(self, uid):
        return f"u_{uid}"

    def _get_m_node(self, mid):
        return f"m_{mid}"

    def build_graph(self):
        # Filter dataframe for high quality ratings
        good_ratings = self.df[self.df['rating'] >= self.min_rating]

        for _, row in good_ratings.iterrows():
            uid = int(row['userId'])
            mid = int(row['movieId'])
            
            u_node = self._get_u_node(uid)
            m_node = self._get_m_node(mid)

            # Add nodes if they don't exist
            if not self.G.has_node(u_node):
                self.G.add_node(u_node, type='user', id=uid)
            
            if not self.G.has_node(m_node):
                self.G.add_node(m_node, type='movie', id=mid)

            # Link user to movie
            self.G.add_edge(u_node, m_node)

    def get_user_movies(self, uid):
        u_node = self._get_u_node(uid)
        if u_node not in self.G:
            return set()
        
        # Get movie IDs from neighbor nodes
        return {self.G.nodes[n]['id'] for n in self.G.neighbors(u_node)}

    def recommend(self, uid, limit=10):
        u_node = self._get_u_node(uid)
        
        if u_node not in self.G:
            return []

        # Get movies watched by target user
        my_movies = set(self.G.neighbors(u_node))
        
        # Find other users who watched the same movies
        similar_users = set()
        for m_node in my_movies:
            for other_u in self.G.neighbors(m_node):
                if other_u != u_node:
                    similar_users.add(other_u)

        scores = defaultdict(float)

        # Calculate Jaccard similarity with those users
        for other_u in similar_users:
            their_movies = set(self.G.neighbors(other_u))
            
            intersection = len(my_movies & their_movies)
            union = len(my_movies | their_movies)
            
            jaccard_sim = intersection / union if union > 0 else 0

            if jaccard_sim > 0:
                # Add weight to movies they liked that target hasn't seen
                for m_node in their_movies:
                    if m_node not in my_movies:
                        mid = self.G.nodes[m_node]['id']
                        scores[mid] += jaccard_sim

        # Sort by highest score
        results = sorted(scores.items(), key=lambda x: x[1], reverse=True)
        return results[:limit]

    def get_stats(self):
        return {
            'nodes': self.G.number_of_nodes(),
            'edges': self.G.number_of_edges()
        }