**High-Performance Voting & Ranking Platform (with Custom C Caching Layer)

This project is a full-stack, high-performance voting and ranking platform built to demonstrate real-world applications of Data Structures and Algorithms (DSA) in combination with custom cache logic.
It combines modern web development with low-level memory-efficient C code to deliver blazing-fast operations for ranking, recommendation, and data storage.

Backend - Nodejs and Firebase 
Handles API routes, authentication, and database

C Middleware -	Custom C-based Cache Engine	
Manages in-memory data structures and eviction

Core Architecture & DSA Strategy

1. HashMaps (Custom)
Used for fast O(1) retrievals in both session and content layers.

a. User Tokens HashMap
Key: user_id or token

Value: Session metadata (e.g., login time, roles, preferences)

Purpose: Quickly validate tokens and manage sessions in constant time.

b. Movie Data HashMap
Key: movie_id

Value: Metadata like title, genre, vote count, last accessed, etc.

Purpose: Serve as the primary indexable source of movie-related information in memory.

2. Sorted Set (Custom)
A data structure similar to Redis' Sorted Set (ZSet), used for ranking.

Key: movie_id

Score: Ranking score (e.g., total votes, upvotes minus downvotes)

Operations Supported:

Top N queries (e.g., getTopKMovies)

Incremental score updates

Use Case: Display the top-voted or most-trending movies in real time.

 3. Priority Queue (Min Heap) — for Eviction Policy
Implements Least Recently Used (LRU) or Least Frequently Used (LFU) eviction strategies.

Element: Cached movie data block

Priority: Frequency count or last-access timestamp

Use Case:

Automatically evict stale or infrequently accessed movie data.

Optimize limited in-memory space using cache eviction logic.

Backed By: A Min Heap or Dual Priority Queue for O(log n) insert/remove.

