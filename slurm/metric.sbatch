#!/bin/bash
#SBATCH --job-name=lab10
#SBATCH --gres=gpu:1
#SBATCH --cpus-per-task=8
#SBATCH --mem=16G
#SBATCH --time=00:05:00
#SBATCH --output=out/lab10-%j.out
set -euo pipefail
cd "$SLURM_SUBMIT_DIR"
PROGRAM=metric
ARGS="data/part.obj out/simplified.obj --compare"
if [[ ! "$PROGRAM" =~ ^(hello|trace|metric|simplify|judge|check)$ ]]; then
  echo "Unknown PROGRAM: $PROGRAM" >&2; exit 1
fi
if [[ ! -x "./build-gpu/$PROGRAM" ]]; then
  echo "build it on the login node first: make -f Makefile.gpu" >&2; exit 1
fi
# Split simple whitespace-separated flags without evaluating shell commands.
read -r -a run_args <<< "$ARGS"
"./build-gpu/$PROGRAM" "${run_args[@]}"
