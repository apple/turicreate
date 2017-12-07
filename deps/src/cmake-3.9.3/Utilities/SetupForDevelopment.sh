#!/usr/bin/env bash

cd "${BASH_SOURCE%/*}/.." &&
Utilities/GitSetup/setup-user && echo &&
Utilities/GitSetup/setup-hooks && echo &&
Utilities/GitSetup/tips

# Rebase master by default
git config rebase.stat true
git config branch.master.rebase true

# Record the version of this setup so Git/pre-commit can check it.
SetupForDevelopment_VERSION=2
git config hooks.SetupForDevelopment ${SetupForDevelopment_VERSION}
