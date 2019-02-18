#
# A more complex test case
#
cmake_minimum_required(VERSION 2.4)
cmake_policy(SET CMP0054 NEW)
project (Complex)

# Test that renaming a built-in works when configured multiple times.
message("message")
function(message)
  _message(${ARGN})
endfunction()
message("message")

# Try setting a new policy.  The IF test is for coverage.
if(POLICY CMP0003)
  cmake_policy(SET CMP0003 NEW)

  cmake_policy(GET CMP0003 P3)
  if(NOT "${P3}" STREQUAL "NEW")
    message(FATAL_ERROR "cmake_policy(GET) did not report NEW!")
  endif()
endif()

# It is not recommended to set a policy to OLD, but this test
# covers the OLD behavior of some policies.
foreach(p
    CMP0029
    CMP0032
    CMP0033
    CMP0034
    CMP0043
    CMP0050
    )
  if(POLICY ${p})
    cmake_policy(SET ${p} OLD)
  endif()
endforeach()

# Test building without per-rule echo lines in Makefiles.
set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)

set(CPACK_SOURCE_IGNORE_FILES "~$;^C:/hoffman/My Builds/testcase.*/CVS/;^C:/hoffman/My Builds/testcase.*/\\.svn/;^C:/hoffman/My Builds/testcase.*/sweigart/;^C:/hoffman/My Builds/testcase/www/eospaper/solution/eos2001/;^C:/hoffman/My Builds/testcase/www/eospaper/solution/opal_tables_new/;^C:/hoffman/My Builds/testcase/COPYING$;^C:/hoffman/My Builds/testcase/INSTALL$;^C:/hoffman/My Builds/testcase/Makefile$;^C:/hoffman/My Builds/testcase/Makefile\\.in$;^C:/hoffman/My Builds/testcase/.*\\.lo$;^C:/hoffman/My Builds/testcase/.*\\.la$;^C:/hoffman/My Builds/testcase/mkinstalldirs$;^C:/hoffman/My Builds/testcase/missing$;^C:/hoffman/My Builds/testcase/ltmain\\.sh$;^C:/hoffman/My Builds/testcase/libtool$;^C:/hoffman/My Builds/testcase/install-sh$;^C:/hoffman/My Builds/testcase/configure$;^C:/hoffman/My Builds/testcase/config\\.sub$;^C:/hoffman/My Builds/testcase/config\\.status$;^C:/hoffman/My Builds/testcase/config\\.log$;^C:/hoffman/My Builds/testcase/config\\.guess$;^C:/hoffman/My Builds/testcase/autom4te\\.cache$;^C:/hoffman/My Builds/testcase/aclocal\\.m4$;^C:/hoffman/My Builds/testcase/depcomp$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/0\\.075\\.model_cassisi_eos1_10$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/0\\.075\\.model_cassisi_eos1_10_corr$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/0\\.1\\.model_cassisi_eos1$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/0\\.1\\.model_cassisi_scvh$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/0\\.1\\.modelc$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/0\\.3\\.modelc$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/1\\.0\\.modelc$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/1\\.0\\.rgbtip\\.modelc$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/1\\.0\\.zahb\\.modelc$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/0\\.1\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/1\\.0\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/0\\.3\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/0\\.085\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/.*\\.ps$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange\\.mem$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange\\.aux$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange\\.log$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange\\.dvi$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange\\.tex\\.bak$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/head\\.tmp$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/body\\.tmp$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/prior-dvi\\.aux$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/j10\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/j12\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/j16\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/j20\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/j22\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/j26\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/j30\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/j32\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/j36\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/k10\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/k12\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/k20\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/k22\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/k30\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/k32\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/1_exchange_dgamma1\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/1_exchange_dlnp\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/2_exchange_dgamma1\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/2_exchange_dlnp\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/linear_exchange_dgamma1\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/linear_exchange_dlnp\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/noexchange_dgamma1\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/noexchange_dlnp\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/nr_exchange_dgamma1\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/nr_exchange_dlnp\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/series_exchange_dgamma1\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/series_exchange_dlnp\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_JNR_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_Jseries_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_KNR_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_Kseries_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check34_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check35_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check36_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check43_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check44_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check45_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check46_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check47_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check48_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/tc_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/.*\\.pyc$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/context\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/make\\.out.*$;^C:/hoffman/My Builds/testcase/www/Makefile$;^C:/hoffman/My Builds/testcase/www/Makefile\\.in$;^C:/hoffman/My Builds/testcase/src/.*\\.flc$;^C:/hoffman/My Builds/testcase/src/Makefile$;^C:/hoffman/My Builds/testcase/src/Makefile\\.in$;^C:/hoffman/My Builds/testcase/src/\\.deps$;^C:/hoffman/My Builds/testcase/src/\\.libs$;^C:/hoffman/My Builds/testcase/src/.*\\.la$;^C:/hoffman/My Builds/testcase/src/.*\\.lo$;^C:/hoffman/My Builds/testcase/src/make\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/statef.*\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/0\\.1\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/0\\.1\\.model_13$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/0\\.1\\.model_23$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/0\\.3\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/0\\.3\\.model_13$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/0\\.3\\.model_23$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/1\\.0\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/1\\.0\\.model_13$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/1\\.0\\.model_15$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/1\\.0\\.model_23$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/1\\.0\\.model_rel$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/hot_post_agb\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/rgb_tip\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/rgbtip\\.1\\.0\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/rgbtip\\.1\\.0\\.model_13$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/rgbtip\\.1\\.0\\.model_23$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/start_shellflash\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/white_dwarf\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/zahb\\.1\\.0\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/zahb\\.1\\.0\\.model_13$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/zahb\\.1\\.0\\.model_23$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/model-loci/zahb\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/dh/dgamma1$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/dh/dlnp$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/dh/statef_compare\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/fermi_dirac_approx/15gamma1\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/fermi_dirac_approx/15lnp\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/fermi_dirac_approx/23gamma1\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/fermi_dirac_approx/23lnp\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/thermodynamic_consistency/.*\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/thermodynamic_consistency/.*\\.results$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/pteh/dgamma1$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/pteh/dlnp$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/pteh/statef_compare\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/newversion_grid/.*\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/newversion_grid/.*\\.err$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/.*\\.ps$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/.*\\.pyc$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_fit\\.aux$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_fit\\.dvi$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_fit\\.log$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/body\\.tmp$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/head\\.tmp$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/prior-dvi\\.aux$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/3order_data\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/5order_data\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/8order_data\\.tex$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_check8_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_check1_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_check3_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_check5_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/effo_check3_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/effoo_check3_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/fda15gamma1\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/fda15lnp\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/fda23gamma1\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/fda23lnp\\.yplot$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/tc_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/make\\.out.*$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/thermodynamic_consistency/statef_compare\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/thermodynamic_consistency/tc\\.results$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/opal_solar/opal_compare_model\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/opal_solar/opal_compare_solar\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/opal_solar/opal_solar\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/opal_solar/opal_solar_1995\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/opal_solar/statef_opal_model\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/opal_solar/statef_opal_model_1995\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/purehe_newversion_grid/.*\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/purehe_newversion_grid/.*\\.err$;^C:/hoffman/My Builds/testcase/include/Makefile\\.in$;^C:/hoffman/My Builds/testcase/include/Makefile$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/model-loci/0\\.1\\.model_pteh$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/model-loci/1\\.0\\.model_eos1a-eos1$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/model-loci/1\\.0\\.model_pteh$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/model-loci/statef_model_0\\.1\\.model_pteh\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/model-loci/statef_model_1\\.0\\.model_eos1a-eos1\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/model-loci/statef_model_1\\.0\\.model_pteh\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/context/contour\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/context/eos_grid\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/context/statef_grid\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/thermodynamic_consistency/fort\\.91$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/thermodynamic_consistency/statef_compare\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/nocoulomb/dgamma1$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/nocoulomb/dlnp$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/nocoulomb/statef_compare\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/context$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/oldversion_grid$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/2005Ap&SS\\.298\\.\\.135S\\.pdf$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/2007Ap&SS\\.307\\.\\.263C\\.pdf$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/astro-ph\\.9909168_eprint_submitted_to_High_Press\\.Res\\.16,331\\.pdf$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/.*ps.*$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/.*\\.pyc$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/convergence\\.aux$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/convergence\\.dvi$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/convergence\\.log$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/body\\.tmp$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/head\\.tmp$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/prior-dvi\\.aux$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/statef_grid-newversion$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/context\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/pureh_context\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/purehe_context\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/old$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/make\\.out.*$;^C:/hoffman/My Builds/testcase/utils/.*\\.flc$;^C:/hoffman/My Builds/testcase/utils/Makefile$;^C:/hoffman/My Builds/testcase/utils/Makefile\\.in$;^C:/hoffman/My Builds/testcase/utils/\\.deps$;^C:/hoffman/My Builds/testcase/utils/\\.libs$;^C:/hoffman/My Builds/testcase/utils/.*\\.la$;^C:/hoffman/My Builds/testcase/utils/.*\\.lo$;^C:/hoffman/My Builds/testcase/utils/free_eos_test$;^C:/hoffman/My Builds/testcase/utils/test_rosenbrock$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_check/eff_check1\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_check/eff_check3\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_check/eff_check5\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_check/eff_check8\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_check/eff_checknr\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_check/effo_check3\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/eff_check/effoo_check3\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence_20070613$;^C:/hoffman/My Builds/testcase/www/eospaper/text$;^C:/hoffman/My Builds/testcase/www/eospaper/cassisi_book_fig$;^C:/hoffman/My Builds/testcase/www/eospaper/1\\.1\\.0$;^C:/hoffman/My Builds/testcase/www/eospaper/2\\.0\\.0$;^C:/hoffman/My Builds/testcase/www/eospaper/1\\.2\\.0$;^C:/hoffman/My Builds/testcase/www/eospaper/1\\.3\\.0$;^C:/hoffman/My Builds/testcase/www/eospaper/1\\.4\\.0$;^C:/hoffman/My Builds/testcase/www/eospaper/1\\.5\\.0$;^C:/hoffman/My Builds/testcase/www/eospaper/1\\.6\\.0$;^C:/hoffman/My Builds/testcase/www/eospaper/figures$;^C:/hoffman/My Builds/testcase/www/eospaper/old$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/.*\\.ps.*$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/coulomb\\.aux$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/coulomb\\.dvi$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/coulomb\\.log$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/.*\\.pyc$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/body\\.tmp$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/head\\.tmp$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/prior-dvi\\.aux$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/context\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/dh_dgamma1_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/dh_dlnp_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/dhtau_dgamma1_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/dhtau_dlnp_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/nocoulomb_dgamma1_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/nocoulomb_dlnp_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/pteh_dgamma1_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/pteh_dlnp_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/make\\.out.*$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/exchange_JNR\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/exchange_Jseries\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/exchange_KNR\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/exchange_Kseries\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/exchange_check34\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/exchange_check35\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/exchange_check36\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/exchange_check44\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/exchange_check45\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/exchange_check46\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/statef_compare_1_exchange\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/statef_compare_2_exchange\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/statef_compare_linear_exchange\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/statef_compare_noexchange\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/statef_compare_nr_exchange\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/statef_compare_series_exchange\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/1_exchange_dgamma1\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/noexchange_dlnp\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/nr_exchange_dgamma1\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/series_exchange_dlnp\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/series_exchange_dgamma1\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/linear_exchange_dlnp\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/2_exchange_dgamma1\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/nr_exchange_dlnp\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/linear_exchange_dgamma1\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/noexchange_dgamma1\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/1_exchange_dlnp\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/exchange_check/2_exchange_dlnp\\.gnuplot$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/pureh_newversion_grid/.*\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/convergence/pureh_newversion_grid/.*\\.err$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/dhtau/dgamma1$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/dhtau/dlnp$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/dhtau/statef_compare\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/statef_model_0\\.1\\.model\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/statef_model_0\\.3\\.model\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/statef_model_1\\.0\\.model\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/statef_model_1\\.0\\.model_linear\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/statef_model_1\\.0\\.model_noexchange\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/statef_model_1\\.0\\.model_nr\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/statef_model_1\\.0\\.rgbtip\\.model\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/statef_model_1\\.0\\.zahb\\.model\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/1\\.0\\.zahb\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/1\\.0\\.rgbtip\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/1\\.0\\.model_linear$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/1\\.0\\.model_noexchange$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/1\\.0\\.model_nr$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/0\\.1\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/1\\.0\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/model-loci/0\\.3\\.model$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/context/contour\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/context/eos_grid\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/exchange/context/statef_grid\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/gong/delta\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/gong/m0085eos1gong\\.ascii$;^C:/hoffman/My Builds/testcase/www/eospaper/eff_fit/rho-T-loci/gong/m0085eos2gong\\.ascii$;^C:/hoffman/My Builds/testcase/www/eospaper/coulomb/coulomb_adjust/coulomb_adjust\\.out$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/.*\\.ps$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/.*\\.pyc$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/head\\.tmp$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/body\\.tmp$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/prior-dvi\\.aux$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/solution\\.aux$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/solution\\.log$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/solution\\.dvi$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/rtc_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/tc_yplot\\.in$;^C:/hoffman/My Builds/testcase/www/eospaper/solution/make\\.out.*$")

#
# Define a macro
#
macro(ASSERT value msg)
  if (NOT ${value})
    message ("Assertion failure:" ${msg} )
  endif ()
endmacro()

# invoke the macro
ASSERT(Complex_BINARY_DIR "The PROJECT command is broken")

#
# Define a var args macro, it must take two or four args
#
macro(TEST_ARGC value1 value2)
  add_definitions(${value1} ${value2})
  if (${ARGC} EQUAL 4)
    add_definitions(${ARGV2} ${ARGV3})
  endif ()
endmacro()

# invoke the macro
TEST_ARGC(-DCMAKE_ARGV1 -DCMAKE_ARGV2 -DCMAKE_ARGV3 -DCMAKE_ARGV4)

macro(TEST_VAR_ARG fa)
  if("${ARGV}" STREQUAL "1;2;3")
    message(STATUS "ARGV works")
  else()
    message(FATAL_ERROR "ARGV does not work; got \"${ARGV}\" instead of \"1;2;3\"")
  endif()
  if("${ARGN}" STREQUAL "2;3")
    message(STATUS "ARGN works")
  else()
    message(FATAL_ERROR "ARGV does not work; got \"${ARGN}\" instead of \"2;3\"")
  endif()
endmacro()

TEST_VAR_ARG(1 2 3)

# Floating-point comparison test.
if(2.4 LESS 2.4)
  message(FATAL_ERROR "Failed: 2.4 LESS 2.4")
endif()
if(2.4 GREATER 2.4)
  message(FATAL_ERROR "Failed: 2.4 GREATER 2.4")
endif()
if(NOT 2.4 EQUAL 2.4)
  message(FATAL_ERROR "Failed: NOT 2.4 EQUAL 2.4")
endif()
if(NOT 2.4 LESS_EQUAL 2.4)
  message(FATAL_ERROR "Failed: NOT 2.4 LESS_EQUAL 2.4")
endif()
if(NOT 2.4 GREATER_EQUAL 2.4)
  message(FATAL_ERROR "Failed: NOT 2.4 GREATER_EQUAL 2.4")
endif()

if(CMAKE_SYSTEM MATCHES "OSF1-V")
  if(NOT CMAKE_COMPILER_IS_GNUCXX)
     string(APPEND CMAKE_CXX_FLAGS " -timplicit_local -no_implicit_include ")
  endif()
endif()


add_definitions(-DCMAKE_IS_FUN)
add_definitions(-DCMAKE_IS_REALLY_FUN)
set_property(DIRECTORY
  PROPERTY COMPILE_DEFINITIONS_RELEASE
  CMAKE_IS_FUN_IN_RELEASE_MODE
  )
set_property(DIRECTORY
  PROPERTY COMPILE_DEFINITIONS_RELWITHDEBINFO
  CMAKE_IS_FUN_IN_RELEASE_MODE
  )
set_property(DIRECTORY
  PROPERTY COMPILE_DEFINITIONS_MINSIZEREL
  CMAKE_IS_FUN_IN_RELEASE_MODE
  )

set(TEST_SEP "a b c")
separate_arguments(TEST_SEP)


#
# Include vars from a file and from a cache
#
if (EXISTS ${Complex_SOURCE_DIR}/VarTests.cmake)
  include(${Complex_SOURCE_DIR}/VarTests.cmake)
endif ()
include(fileshouldnotbehere OPTIONAL)
load_cache(${Complex_SOURCE_DIR}/Cache
           EXCLUDE
             CACHE_TEST_VAR_EXCLUDED
           INCLUDE_INTERNALS
             CACHE_TEST_VAR_INTERNAL)

load_cache(${Complex_SOURCE_DIR}/Cache READ_WITH_PREFIX foo CACHE_TEST_VAR2)
if(${fooCACHE_TEST_VAR2} MATCHES bar)
  message("Load cache worked: ${fooCACHE_TEST_VAR2}")
else()
  message(FATAL_ERROR "Load cache with prefix failed: ${fooCACHE_TEST_VAR2}")
endif()



#
# Specify include and lib dirs
# (BEFORE is for coverage)
#
include_directories(
  Library
)

include_directories(BEFORE
  ${Complex_BINARY_DIR}
)
include_directories(SYSTEM Library/SystemDir)

include_regular_expression("^(cmTest|file|sharedFile|test).*$" "^cmMissing")

link_directories(
  ${Complex_BINARY_DIR}/Library
)

#
# check for SET CACHE FORCE
#
set(FORCE_TEST 1 CACHE STRING "a test")
set(FORCE_TEST 0 CACHE STRING "a test" FORCE)

#
# Lib and exe path
#
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${Complex_BINARY_DIR}/lib/static")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${Complex_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${Complex_BINARY_DIR}/bin")

message (Test " " escape " " semi-colon " " \; \;)
#
# Exec program (TODO: test a result)
# Increase coverage.
#
message("\nIgnore this message")
option(NO_EXEC_PROGRAM "Do not test EXEC_PROGRAM" 0)
if (NOT NO_EXEC_PROGRAM)
  exec_program(${CMAKE_COMMAND} ARGS -E echo NO_EXEC_PROGRAM "${Complex_BINARY_DIR}")
else ()
  message("Set this option ON")
endif ()

mark_as_advanced(NO_EXEC_PROGRAM)
mark_as_advanced(CLEAR NO_EXEC_PROGRAM)

# Execute a process.  Add coverage for this command.
execute_process(
  COMMAND ${CMAKE_COMMAND} -E echo "ABCDEFG"
  OUTPUT_VARIABLE TEST_OUT
  )
if("${TEST_OUT}" STREQUAL "ABCDEFG\n")
else()
  message(SEND_ERROR "EXECUTE_PROCESS output test failed: [${TEST_OUT}]")
endif()

# This test has some problems on UNIX systems.  Disabling for now.
#
# execute_process(
#   COMMAND ${CMAKE_COMMAND} -E echo "ABCDEFG"
#   COMMAND /process/does/not/exist
#   OUTPUT_QUIET
#   ERROR_QUIET
#   RESULT_VARIABLE RESULT
#   )
# if("${RESULT}" STREQUAL "0")
#   message(SEND_ERROR
#     "EXECUTE_PROCESS result test failed with RESULT=[${RESULT}]")
# else()
#   message(STATUS "EXECUTE_PROCESS result test passed with RESULT=[${RESULT}]")
# endif()

#
# Create directory.
# The 'complex' executable will then test if this dir exists,
# sadly it won't be able to remove it.
#
make_directory("${Complex_BINARY_DIR}/make_dir")

#
# Test FIND_LIBARY
# Create a dummy empty lib
#
configure_file(
  ${Complex_SOURCE_DIR}/Library/dummy
  ${Complex_BINARY_DIR}/Library/dummylib.lib
  COPYONLY)
foreach (ext ${CMAKE_SHLIB_SUFFIX};.so;.a;.sl
         ${CMAKE_SHARED_LIBRARY_SUFFIX}.2
         ${CMAKE_STATIC_LIBRARY_SUFFIX}.2)
  configure_file(
    ${Complex_SOURCE_DIR}/Library/dummy
    ${Complex_BINARY_DIR}/Library/libdummylib${ext}
    COPYONLY)
endforeach ()

find_library(FIND_DUMMY_LIB
             dummylib
             PATHS
             ${Complex_BINARY_DIR}/Library DOC "find dummy lib")

find_library(FIND_DUMMY_LIB
             NAMES dummylib dummylib2
             PATHS
             ${Complex_BINARY_DIR}/Library DOC "find dummy lib")

# This doesn't work for platforms that have a shared library and an import
# library, like Windows with .dll and .lib. Limit is to ".so" now because it's
# known to work there.
if(CMAKE_SHARED_LIBRARY_SUFFIX STREQUAL ".so")
  find_library(FIND_DUMMY_SHLIB_VERSIONED
               NAMES libdummylib${CMAKE_SHARED_LIBRARY_SUFFIX}.2
               PATHS ${Complex_BINARY_DIR}/Library
               DOC "find versioned dummy shared lib"
               NO_DEFAULT_PATH)

  if(NOT FIND_DUMMY_SHLIB_VERSIONED MATCHES "/libdummylib${CMAKE_SHARED_LIBRARY_SUFFIX}.2")
    message(SEND_ERROR "FIND_DUMMY_SHLIB_VERSIONED is not set correctly: "
            "${FIND_DUMMY_SHLIB_VERSIONED}")
  endif()
endif()

# Static library, should work everywhere
find_library(FIND_DUMMY_STLIB_VERSIONED
             NAMES libdummylib${CMAKE_STATIC_LIBRARY_SUFFIX}.2
             PATHS ${Complex_BINARY_DIR}/Library
             DOC "find versioned dummy static lib"
             NO_DEFAULT_PATH)

if(NOT FIND_DUMMY_STLIB_VERSIONED MATCHES "/libdummylib${CMAKE_STATIC_LIBRARY_SUFFIX}.2")
  message(SEND_ERROR "FIND_DUMMY_STLIB_VERSIONED is not set correctly: "
          "${FIND_DUMMY_STLIB_VERSIONED}")
endif()

#
# Test SET_SOURCE_FILES_PROPERTIES
#
set_source_files_properties(nonexisting_file2
  GENERATED
  ABSTRACT
  WRAP_EXCLUDE
  COMPILE_FLAGS "-foo -bar")

get_source_file_property(FILE_HAS_ABSTRACT nonexisting_file2 ABSTRACT)
get_source_file_property(FILE_HAS_WRAP_EXCLUDE nonexisting_file2 WRAP_EXCLUDE)
get_source_file_property(FILE_COMPILE_FLAGS nonexisting_file2 COMPILE_FLAGS)

set_source_files_properties(nonexisting_file3 PROPERTIES
  GENERATED 1
  ABSTRACT 1
  WRAP_EXCLUDE 1
  COMPILE_FLAGS "-foo -bar")
get_source_file_property(FILE_HAS_ABSTRACT nonexisting_file3 ABSTRACT)
get_source_file_property(FILE_HAS_WRAP_EXCLUDE nonexisting_file3 WRAP_EXCLUDE)
get_source_file_property(FILE_COMPILE_FLAGS nonexisting_file3 COMPILE_FLAGS)

#
# Test registry (win32)
# Create a file, put its path in a registry key, try to find the file in that
# path using that registry key, then remove the file and the key
#
if (WIN32)
  if (NOT UNIX)
    set(dir "${Complex_BINARY_DIR}/registry_dir")
    set(file "registry_test_dummy")
    set(hkey "HKEY_CURRENT_USER\\Software\\Kitware\\CMake\\Tests\\Complex;registry_test")
    configure_file(
      ${Complex_SOURCE_DIR}/Library/dummy
      "${dir}/${file}"
      COPYONLY)
    exec_program(${CMAKE_COMMAND} ARGS "-E write_regv \"${hkey}\" \"${dir}\"")
    find_path(REGISTRY_TEST_PATH
      ${file}
      "[${hkey}]" DOC "Registry_Test_Path")
    exec_program(${CMAKE_COMMAND} ARGS "-E delete_regv \"${hkey}\"")
    exec_program(${CMAKE_COMMAND} ARGS "-E remove \"${dir}/${file}\"")
  endif ()
endif ()

#
# Test a set and a remove
#
set(REMOVE_STRING a b c d e f)
set(removeVar1 c e)
remove(REMOVE_STRING ${removeVar1} f)

#
# Test an IF inside a FOREACH.
#
foreach(x "a")
  if(${x} MATCHES "a")
    # Should always execute.
    set(IF_INSIDE_FOREACH_THEN_EXECUTED 1)
  else()
    # Should never execute.
    set(IF_INSIDE_FOREACH_ELSE_EXECUTED 1)
  endif()
endforeach()

# test WHILE command
set (while_var 1)
while (while_var LESS 1000)
  set(while_var ${while_var}0)
endwhile()

set(SHOULD_BE_ZERO )
set(SHOULD_BE_ONE 1)
set(SHOULD_BE_ZERO_AND_INDENTED )
set(SHOULD_BE_ONE_AND_INDENTED 1)

# test elseif functionality, the mess below tries to catch problem
# of clauses being executed early or late etc
set (RESULT 3)
if (RESULT EQUAL 1)
  if (RESULT EQUAL 2)
    set (ELSEIF_RESULT 1)
  elseif (RESULT EQUAL 3)
    set (ELSEIF_RESULT 1)
  endif ()
elseif (RESULT EQUAL 2)
  set (ELSEIF_RESULT 1)
elseif (RESULT EQUAL 3)
  if (RESULT EQUAL 2)
    set (ELSEIF_RESULT 1)
  elseif (RESULT EQUAL 3)
    if (NOT ELSEIF_RESULT EQUAL 1)
      set (ELSEIF_RESULT 2)
    endif ()
  endif ()
elseif (RESULT EQUAL 4)
  if (RESULT EQUAL 2)
    set (ELSEIF_RESULT 1)
  elseif (RESULT EQUAL 3)
    set (ELSEIF_RESULT 1)
  endif ()
else ()
  if (RESULT EQUAL 2)
    set (ELSEIF_RESULT 1)
  elseif (RESULT EQUAL 3)
    set (ELSEIF_RESULT 1)
  endif ()
endif ()

if (NOT ELSEIF_RESULT EQUAL 2)
  set (ELSEIF_RESULT 0)
endif ()

# test handling of parenthetical groups in conditionals
if (2 GREATER 1 AND (4 LESS 3 OR 5 LESS 6) AND NOT (7 GREATER 8))
  set(CONDITIONAL_PARENTHESES 1)
endif()


#
# Configure file
# (plug vars to #define so that they can be tested)
#
configure_file(
  ${Complex_SOURCE_DIR}/cmTestConfigure.h.in
  ${Complex_BINARY_DIR}/cmTestConfigure.h)

set(STRING_WITH_QUOTES "\"hello world\"")
# test CONFIGURE_FILE with ESCAPE_QUOTES on
configure_file(
  ${Complex_SOURCE_DIR}/cmTestConfigureEscape.h.in
  ${Complex_BINARY_DIR}/cmTestConfigureEscape.h ESCAPE_QUOTES)

# Test regular expression commands.
string(REGEX MATCH "b" RESULT "abc")
if(NOT RESULT)
  message(SEND_ERROR "string(REGEX MATCH ... ) test failed.")
endif()
string(REGEX MATCHALL "b" RESULT "abcb")
if(NOT RESULT)
  message(SEND_ERROR "string(REGEX MATCHALL ... ) test failed.")
endif()
string(REGEX REPLACE ".([bd])." "[\\1]" RESULT "a(b)c(d)e")
if(NOT RESULT STREQUAL "a[b]c[d]e")
  message(SEND_ERROR
    "string(REGEX REPLACE ... ) test failed (\"${RESULT}\" v. \"a[b]c[d]e\")")
endif()

#
# This tests needs Ansi C++98
#
set(CMAKE_CXX_STANDARD 98)
#
# GNU extensions are needed for stricmp() on Windows.
#
set(CMAKE_CXX_EXTENSIONS TRUE)

# Clang/C2 in C++98 mode cannot properly handle some of MSVC headers
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
    CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  set(CMAKE_CXX_STANDARD 11)
endif()

#
# Create the libs and the main exe
#
add_subdirectory(Library)
add_subdirectory(Executable)
subdir_depends(Executable Library)
export_library_dependencies(${Complex_BINARY_DIR}/ComplexLibraryDepends.cmake)
include(${Complex_BINARY_DIR}/ComplexLibraryDepends.cmake OPTIONAL)
