# Coreutils Generation Contest Rules

Welcome to the **Coreutils Generation Contest**, where participants leverage large language models (LLMs) to generate efficient and reliable implementations of core UNIX utilities. The contest aims to evaluate and push the boundaries of AI-assisted software development, with a focus on legibility, correctness, and performance.

This is a listing of most coreutils implementations:
* [UNIX System 5-7 repo](https://minnie.tuhs.org/cgi-bin/utree.pl?file=V7/usr/src/cmd)
* [Rust MIT repo](https://github.com/uutils/coreutils)
* [GNU repo](https://github.com/coreutils/coreutils)
* [Apple repo](https://opensource.apple.com/tarballs/shell_cmds/)
* [BSD repo](https://github.com/freebsd/freebsd-src/tree/master/bin)
* [Illiuminos (Open Solaris) repo](https://github.com/illumos/illumos-gate/tree/master/usr/src/cmd)


---

## Input 

The input will be a set of historic coreutil man pages. This will change from contest to contest. 

## Categories

Participants may enter in one of the following categories:

### 1. **Stock**
- Submissions **must** use one of the [Ollama](https://ollama.com/) hosted models without any additional fine-tuning.
- Emphasis is on **prompt engineering** and model selection.
- Entries should be capable of running "out of the box" with the provided model.

### 2. **Indy**
- Submissions **must** run on [llama.cpp](https://github.com/ggerganov/llama.cpp), an optimized inference engine for LLMs.
- Please limit models to about 70b. The test machine is a Mac Studio with 64GB RAM.
- A portion of the compute budget may be used for fine-tuning with tools like:
  - [Unsloth](https://github.com/unslothai/unsloth)
  - Built-in fine-tuning capabilities of `llama.cpp`
- Customization and optimization of the model are encouraged to maximize accuracy and efficiency.

### 3. **Stock Smol**
- Submissions **must** use smaller, accessible models (1B - 7B class) that can run on a **Mac M1** or equivalent hardware.
- The goal is to create viable solutions using lightweight models that are accessible to most users.

---

## Licensing Requirements

All submissions **must** be licensed under the **MIT License**, ensuring openness and usability for all participants and the broader community.

---

## Accepted Languages

Entries can be submitted in any of the following programming languages:

- **C**
- **C++**
- **Rust**
- **Python**
- **Mac M2 ARM assembly**
- **Mac M2 METAL**

---

## Judging Criteria

Submissions will be evaluated based on the following criteria:

1. **Generated Code Legibility (30 points)**
   - Submissions might be normalized with clang-format
   - What is the context window needed to read the code? 
   
3. **Generated Test Suite Compliance (40 points)**
   - We will add historic coreutils MAN pages as specifications the model may reference.
   - Ability to catch same bugs as standard coreutils test suites.

5. **Performance (30 points)**
   - Efficient execution with minimal resource usage.
   - Optimized algorithms and minimal overhead.

### Extra Credit Opportunities
Additional points will be awarded for:

- Valid **integration tests** that expose **bugs or inefficiencies** in major existing coreutils implementations.
- Finding and triggering **performance degradation** or algorithmic weaknesses (e.g., accidental quadratic complexity).
- Formal specifications of coreutil MAN pages for future contests.
- Novel use of operating system calls to improve reliability/performance.
- Novel test suite code leveraging strace/dtrace, gdb/lldb, eBPF, or QEMU to observe operating system calls and stack traces.
- Novel test suite code that emits fuzz targets which break existing coreutil implmementations or generates better code coverage.
- Novel use of [mlx](https://github.com/ml-explore/mlx), [MPI](https://github.com/open-mpi/ompi), pthreads, and GPU to scale coreutils beyond single CPU cores.
- Novel use of generated sympy or z3py to prove correctness or generate optimized code.
- Test suites that exploit novel bugs/regressions in bash or zsh.
---

## Contest Duration

This contest will run **every three months** at [solar solstice/equinox](https://greenwichmeantime.com/longest-day/equinox-solstice-2021-2030/) starting March 20, 2025 until **Artificial Superintelligence (ASI)** is achieved, at which point stock models, without special prompting techniques, are sufficiently advanced that human intervention becomes negligible.

---

## Submission Guidelines

1. Submit your entries by forking this repository and sending a pull request to the quarterly contest fork.
2. Include:
   - Source code.
   - [Bezos six page memo](https://www.sixpagermemo.com) describing your work.
   - A README with instructions on how to run and test the implementation.
   - A submission of the best coreutils package it has generated for each participation category.
   - Generated code should run on a stock Github worker.
3. Clearly state which category your submission belongs to, and the target architectures (x86/ARM, Linux/Mac/Windows)
4. Ensure all dependencies and requirements are clearly specified. Please use minimal dependencies.
5. Please generate [AWK style expectation tests](https://www.cs.princeton.edu/courses/archive/spring01/cs333/awktest.html) that are reusable. [z3test](https://github.com/Z3Prover/z3test) is a well maintained expectation test respository to reference.
---

## Prizes and Recognition

Winners will receive:

- Public recognition in the AI developer community.
- Opportunities for further research collaboration.
- Referrals to [Rust coreutils](https://github.com/uutils/coreutils) and BSD (MacOS) coreutils maintainers so your code hopefully gets put in production.
- Winners are encouraged to refine their memos for submission to [USENIX](https://www.usenix.org/conference/atc25)

---

## Good Luck!

We look forward to seeing your innovative solutions and contributions to pushing AI-generated software development forward.
