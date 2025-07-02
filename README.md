# Systems Programming 2024 Homework Overview

This repository contains my solutions to four homework assignments in NTU Systems Programming 2024. Below are brief descriptions and spec links for each assignment.

---

## Homework 1: Train Booking Server

We implement a read server and a write server for a train booking system, using `poll()` to handle requests simultaneously and file locks to keep record consistency. In addition, we handle connection timeouts and connection issues (such as incomplete messages), making the system more realistic.

**[Spec Link](https://hackmd.io/@EdisonCHEN/sp-hw1)**

---

## Homework 2: Multi-process Friends Tree Record System

We implement a multi-process friends tree record system. Each friend is a single process; we use `fork`, `dup`, and `exec` to create new nodes and manipulate file descriptors, and we use pipes and FIFOs for process communication in the project.

**[Spec Link](https://hackmd.io/@EdisonCHEN/sp-hw2)**

---

## Homework 3: User-Level Thread Library

We implement a user-level thread library, including preemption and context switching with the help of `setjmp` and `longjmp`. In addition, we manage thread synchronization using read-write locks, implement a sleeping and waking mechanism for threads, and handle thread states including running, ready, waiting, and sleeping.

**[Spec Link](https://hackmd.io/@EdisonCHEN/sp-hw3)**

---

## Homework 4: Thread Pool and Parallel Matrix Multiplication

We implement a thread pool with the help of the `pthread` library, to improve efficiency by parallel multiplication of matrices.

**[Spec Link](https://hackmd.io/@EdisonCHEN/sp-hw4)**

---

