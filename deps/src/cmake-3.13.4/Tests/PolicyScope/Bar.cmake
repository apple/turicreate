cmake_minimum_required(VERSION 2.6.3)

# Make sure a policy set differently by our includer is now correct.
cmake_policy(GET CMP0003 cmp)
check(CMP0003 "NEW" "${cmp}")

# Test allowing the top-level file to not have cmake_minimum_required.
cmake_policy(SET CMP0000 OLD)
