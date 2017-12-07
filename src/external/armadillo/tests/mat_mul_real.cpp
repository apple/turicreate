// Copyright 2015 Conrad Sanderson (http://conradsanderson.id.au)
// Copyright 2015 National ICT Australia (NICTA)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------------


#include <numerics/armadillo.hpp>
#include "catch.hpp"

using namespace arma;


TEST_CASE("mat_mul_real_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat A00 = A( 0,0, size(0,0) );
  mat A11 = A( 0,0, size(1,1) );
  mat A22 = A( 0,0, size(2,2) );
  mat A33 = A( 0,0, size(3,3) );
  mat A44 = A( 0,0, size(4,4) );
  mat A55 = A( 0,0, size(5,5) );

  mat B = fliplr(A);

  mat B00 = B( 0,0, size(0,0) );
  mat B11 = B( 0,0, size(1,1) );
  mat B22 = B( 0,0, size(2,2) );
  mat B33 = B( 0,0, size(3,3) );
  mat B44 = B( 0,0, size(4,4) );
  mat B55 = B( 0,0, size(5,5) );

  mat A00_times_B00(0,0);

  mat A11_times_B11 =
    "\
     0.003146066784;\
    ";

  mat A22_times_B22 =
    "\
     0.010304   0.052063;\
     0.024567  -0.037958;\
    ";

  mat A33_times_B33 =
    "\
     0.0013604   0.0534077  -0.0311519;\
     0.0924518  -0.0481622  -0.2813422;\
    -0.1692102   0.0746086   0.3765357;\
    ";

  mat A44_times_B44 =
    "\
    -0.183289   0.120109   0.163034  -0.249241;\
     0.075456  -0.042023  -0.263468  -0.067969;\
    -0.012300   0.017928   0.211522   0.286117;\
    -0.323470   0.163659   0.162149  -0.091062;\
    ";

  mat A55_times_B55 =
    "\
    -0.2160787   0.1649472   0.1999190  -0.1976620  -0.1252585;\
     0.1520715  -0.1467921  -0.3496545  -0.1884897  -0.0492639;\
     0.0053737  -0.0062406   0.1916407   0.2583152   0.0322787;\
    -0.3584056   0.2114322   0.2014480  -0.0361067  -0.0260243;\
    -0.0182371  -0.0207407  -0.0522859  -0.0485276   0.0678171;\
    ";

  REQUIRE( accu(abs( (A00*B00) - A00_times_B00 )) == Approx(0.0) );
  REQUIRE( accu(abs( (A11*B11) - A11_times_B11 )) == Approx(0.0) );
  REQUIRE( accu(abs( (A22*B22) - A22_times_B22 )) == Approx(0.0) );
  REQUIRE( accu(abs( (A33*B33) - A33_times_B33 )) == Approx(0.0) );
  REQUIRE( accu(abs( (A44*B44) - A44_times_B44 )) == Approx(0.0) );
  REQUIRE( accu(abs( (A55*B55) - A55_times_B55 )) == Approx(0.0) );

  mat X;
  REQUIRE_THROWS( X = A22*B44 );
  }



TEST_CASE("mat_mul_real_2")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat A00 = A( 0,0, size(0,0) );
  mat A11 = A( 0,0, size(1,1) );
  mat A22 = A( 0,0, size(2,2) );
  mat A33 = A( 0,0, size(3,3) );
  mat A44 = A( 0,0, size(4,4) );
  mat A55 = A( 0,0, size(5,5) );

  mat B = fliplr(A);

  mat B00 = B( 0,0, size(0,0) );
  mat B11 = B( 0,0, size(1,1) );
  mat B22 = B( 0,0, size(2,2) );
  mat B33 = B( 0,0, size(3,3) );
  mat B44 = B( 0,0, size(4,4) );
  mat B55 = B( 0,0, size(5,5) );

  colvec q0( uword(0) );
  colvec q1 = B11.col(0);
  colvec q2 = B22.col(0);
  colvec q3 = B33.col(0);
  colvec q4 = B44.col(0);
  colvec q5 = B55.col(0);

  rowvec r0( uword(0) );
  rowvec r1 = B11.row(0);
  rowvec r2 = B22.row(0);
  rowvec r3 = B33.row(0);
  rowvec r4 = B44.row(0);
  rowvec r5 = B55.row(0);

  mat A00_times_q0(0,1);

  mat A11_times_q1 =
    "\
     0.0031461;\
    ";

  mat A22_times_q2 =
    "\
     0.010304;\
     0.024567;\
    ";

  mat A33_times_q3 =
    "\
     0.0013604;\
     0.0924518;\
    -0.1692102;\
    ";

  mat A44_times_q4 =
    "\
    -0.183289;\
     0.075456;\
    -0.012300;\
    -0.323470;\
    ";

  mat A55_times_q5 =
    "\
    -0.2160787;\
     0.1520715;\
     0.0053737;\
    -0.3584056;\
    -0.0182371;\
    ";

  mat r0_times_A00(1,0);

  mat r1_times_A11 =
    "\
     0.0031461;\
    ";

  mat r2_times_A22 =
    "\
    -0.0522722   0.0029115;\
    ";

  mat r3_times_A33 =
    "\
     0.190978   0.018376  -0.135230;\
    ";

  mat r4_times_A44 =
    "\
     0.197597   0.026474  -0.126209  -0.234687;\
    ";

  mat r5_times_A55 =
    "\
     0.245991  -0.060162  -0.208409  -0.293470  -0.151911;\
    ";

  REQUIRE( accu(abs( (A00*q0) - A00_times_q0 )) == Approx(0.0) );
  REQUIRE( accu(abs( (A11*q1) - A11_times_q1 )) == Approx(0.0) );
  REQUIRE( accu(abs( (A22*q2) - A22_times_q2 )) == Approx(0.0) );
  REQUIRE( accu(abs( (A33*q3) - A33_times_q3 )) == Approx(0.0) );
  REQUIRE( accu(abs( (A44*q4) - A44_times_q4 )) == Approx(0.0) );
  REQUIRE( accu(abs( (A55*q5) - A55_times_q5 )) == Approx(0.0) );

  REQUIRE( accu(abs( (r0*A00) - r0_times_A00 )) == Approx(0.0) );
  REQUIRE( accu(abs( (r1*A11) - r1_times_A11 )) == Approx(0.0) );
  REQUIRE( accu(abs( (r2*A22) - r2_times_A22 )) == Approx(0.0) );
  REQUIRE( accu(abs( (r3*A33) - r3_times_A33 )) == Approx(0.0) );
  REQUIRE( accu(abs( (r4*A44) - r4_times_A44 )) == Approx(0.0) );
  REQUIRE( accu(abs( (r5*A55) - r5_times_A55 )) == Approx(0.0) );

  mat X;
  REQUIRE_THROWS( X = A22*q4 );
  }



TEST_CASE("mat_mul_real_3")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat A00 = A( 0,0, size(0,0) );
  mat A11 = A( 0,0, size(1,1) );
  mat A22 = A( 0,0, size(2,2) );
  mat A33 = A( 0,0, size(3,3) );
  mat A44 = A( 0,0, size(4,4) );
  mat A55 = A( 0,0, size(5,5) );

  mat B = fliplr(A);

  mat B00 = B( 0,0, size(0,0) );
  mat B11 = B( 0,0, size(1,1) );
  mat B22 = B( 0,0, size(2,2) );
  mat B33 = B( 0,0, size(3,3) );
  mat B44 = B( 0,0, size(4,4) );
  mat B55 = B( 0,0, size(5,5) );

  colvec q0( uword(0) );
  colvec q1 = B11.col(0);
  colvec q2 = B22.col(0);
  colvec q3 = B33.col(0);
  colvec q4 = B44.col(0);
  colvec q5 = B55.col(0);

  rowvec r0( uword(0) );
  rowvec r1 = B11.row(0);
  rowvec r2 = B22.row(0);
  rowvec r3 = B33.row(0);
  rowvec r4 = B44.row(0);
  rowvec r5 = B55.row(0);

  mat q0_t_times_A00(1,0);

  mat q1_t_times_A11 =
    "\
     0.0031461;\
    ";

  mat q2_t_times_A22 =
    "\
     0.018641   0.012473;\
    ";

  mat q3_t_times_A33 =
    "\
     0.242470   0.026703  -0.147065;\
    ";

  mat q4_t_times_A44 =
    "\
     0.368209   0.180551   0.024329  -0.364740;\
    ";

  mat q5_t_times_A55 =
    "\
     0.430191   0.069589  -0.080952  -0.440028  -0.169075;\
    ";

  mat A00_times_r0_t(0,1);

  mat A11_times_r1_t =
    "\
     0.0031461;\
    ";

  mat A22_times_r2_t =
    "\
    -0.022455;\
     0.015005;\
    ";

  mat A33_times_r3_t =
    "\
    -0.032175;\
     0.088781;\
    -0.176522;\
    ";

  mat A44_times_r4_t =
    "\
    -0.041895;\
     0.087886;\
    -0.168262;\
    -0.269064;\
    ";

  mat A55_times_r5_t =
    "\
    -0.067496;\
     0.147706;\
    -0.154463;\
    -0.296340;\
     0.190504;\
    ";

  REQUIRE( accu(abs( (q0.t()*A00) - q0_t_times_A00 )) == Approx(0.0) );
  REQUIRE( accu(abs( (q1.t()*A11) - q1_t_times_A11 )) == Approx(0.0) );
  REQUIRE( accu(abs( (q2.t()*A22) - q2_t_times_A22 )) == Approx(0.0) );
  REQUIRE( accu(abs( (q3.t()*A33) - q3_t_times_A33 )) == Approx(0.0) );
  REQUIRE( accu(abs( (q4.t()*A44) - q4_t_times_A44 )) == Approx(0.0) );
  REQUIRE( accu(abs( (q5.t()*A55) - q5_t_times_A55 )) == Approx(0.0) );

  REQUIRE( accu(abs( (A00*r0.t()) - A00_times_r0_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (A11*r1.t()) - A11_times_r1_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (A22*r2.t()) - A22_times_r2_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (A33*r3.t()) - A33_times_r3_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (A44*r4.t()) - A44_times_r4_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (A55*r5.t()) - A55_times_r5_t )) == Approx(0.0) );

  REQUIRE( accu(abs( (q0.t().eval()*A00) - q0_t_times_A00 )) == Approx(0.0) );
  REQUIRE( accu(abs( (q1.t().eval()*A11) - q1_t_times_A11 )) == Approx(0.0) );
  REQUIRE( accu(abs( (q2.t().eval()*A22) - q2_t_times_A22 )) == Approx(0.0) );
  REQUIRE( accu(abs( (q3.t().eval()*A33) - q3_t_times_A33 )) == Approx(0.0) );
  REQUIRE( accu(abs( (q4.t().eval()*A44) - q4_t_times_A44 )) == Approx(0.0) );
  REQUIRE( accu(abs( (q5.t().eval()*A55) - q5_t_times_A55 )) == Approx(0.0) );

  REQUIRE( accu(abs( (A00*r0.t().eval()) - A00_times_r0_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (A11*r1.t().eval()) - A11_times_r1_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (A22*r2.t().eval()) - A22_times_r2_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (A33*r3.t().eval()) - A33_times_r3_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (A44*r4.t().eval()) - A44_times_r4_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (A55*r5.t().eval()) - A55_times_r5_t )) == Approx(0.0) );

  mat X;
  REQUIRE_THROWS( X = A22*r4.t() );
  }



TEST_CASE("mat_mul_real_4")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat A44 = A( 0,0, size(4,4) );

  mat B = fliplr(A);

  mat B44 = B( 0,0, size(4,4) );

  //

  mat A44_times_B44 =
    "\
    -0.183289   0.120109   0.163034  -0.249241;\
     0.075456  -0.042023  -0.263468  -0.067969;\
    -0.012300   0.017928   0.211522   0.286117;\
    -0.323470   0.163659   0.162149  -0.091062;\
    ";

  mat A44_t_times_B44 =
    "\
     0.368209   0.042669  -0.389048  -0.064607;\
     0.180551  -0.065855  -0.277385   0.174015;\
     0.024329  -0.087178  -0.051312   0.331590;\
    -0.364740   0.130904   0.576774  -0.051312;\
    ";

  mat A44_times_B44_t =
    "\
    -0.041895   0.134869  -0.160929  -0.238593;\
     0.087886   0.046536  -0.271674   0.193369;\
    -0.168262  -0.103699   0.485413  -0.110945;\
    -0.269064   0.171674  -0.055826  -0.290325;\
    ";

  mat A44_t_times_B44_t =
    "\
     0.1975972   0.1038113  -0.0989840   0.3116527;\
     0.0264745  -0.0354272   0.0283701   0.2685396;\
    -0.1262086  -0.1262987   0.2567470   0.1142194;\
    -0.2346872   0.0086687   0.2740562  -0.5237682;\
    ";

  //

  mat two_times_A44_times_B44 =
    "\
    -0.366578   0.240218   0.326067  -0.498482;\
     0.150911  -0.084045  -0.526936  -0.135939;\
    -0.024600   0.035856   0.423045   0.572234;\
    -0.646941   0.327319   0.324297  -0.182123;\
    ";

  mat two_times_A44_t_times_B44 =
    "\
     0.736418   0.085337  -0.778096  -0.129215;\
     0.361101  -0.131709  -0.554770   0.348029;\
     0.048657  -0.174357  -0.102624   0.663181;\
    -0.729480   0.261807   1.153548  -0.102624;\
    ";

  mat two_times_A44_times_B44_t =
    "\
    -0.083789   0.269738  -0.321857  -0.477186;\
     0.175772   0.093072  -0.543347   0.386739;\
    -0.336525  -0.207399   0.970827  -0.221889;\
    -0.538127   0.343348  -0.111652  -0.580649;\
    ";

  mat two_times_A44_t_times_B44_t =
    "\
     0.395194   0.207623  -0.197968   0.623305;\
     0.052949  -0.070854   0.056740   0.537079;\
    -0.252417  -0.252597   0.513494   0.228439;\
    -0.469374   0.017337   0.548112  -1.047536;\
    ";

  //

  mat A44_times_two_times_B44 =
    "\
    -0.366578   0.240218   0.326067  -0.498482;\
     0.150911  -0.084045  -0.526936  -0.135939;\
    -0.024600   0.035856   0.423045   0.572234;\
    -0.646941   0.327319   0.324297  -0.182123;\
    ";

  mat A44_t_times_two_times_B44 =
    "\
     0.736418   0.085337  -0.778096  -0.129215;\
     0.361101  -0.131709  -0.554770   0.348029;\
     0.048657  -0.174357  -0.102624   0.663181;\
    -0.729480   0.261807   1.153548  -0.102624;\
    ";

  mat A44_times_two_times_B44_t =
    "\
    -0.083789   0.269738  -0.321857  -0.477186;\
     0.175772   0.093072  -0.543347   0.386739;\
    -0.336525  -0.207399   0.970827  -0.221889;\
    -0.538127   0.343348  -0.111652  -0.580649;\
    ";

  mat A44_t_times_two_times_B44_t =
    "\
     0.395194   0.207623  -0.197968   0.623305;\
     0.052949  -0.070854   0.056740   0.537079;\
    -0.252417  -0.252597   0.513494   0.228439;\
    -0.469374   0.017337   0.548112  -1.047536;\
    ";

  //

  mat two_times_A44_times_two_times_B44 =
    "\
    -0.733157   0.480435   0.652135  -0.996965;\
     0.301822  -0.168090  -1.053872  -0.271877;\
    -0.049201   0.071711   0.846089   1.144468;\
    -1.293881   0.654637   0.648595  -0.364247;\
    ";

  mat two_times_A44_t_times_two_times_B44 =
    "\
     1.472836   0.170675  -1.556191  -0.258430;\
     0.722203  -0.263419  -1.109539   0.696059;\
     0.097314  -0.348714  -0.205248   1.326362;\
    -1.458960   0.523615   2.307096  -0.205248;\
    ";

  mat two_times_A44_times_two_times_B44_t =
    "\
    -0.167578003928   0.539476906232  -0.643714124056  -0.95437155377;\
     0.351543868312   0.186144110728  -1.086694470368   0.77347794529;\
    -0.673049184916  -0.414797273204   1.941653137076  -0.44377813703;\
    -1.076254284828   0.686695294300  -0.223303479708  -1.16129844700;\
    ";

  mat two_times_A44_t_times_two_times_B44_t =
    "\
     0.790389   0.415245  -0.395936   1.246611;\
     0.105898  -0.141709   0.113480   1.074158;\
    -0.504834  -0.505195   1.026988   0.456878;\
    -0.938749   0.034675   1.096225  -2.095073;\
    ";

  //

  REQUIRE( accu(abs( A44     * B44     - A44_times_B44     )) == Approx(0.0) );
  REQUIRE( accu(abs( A44.t() * B44     - A44_t_times_B44   )) == Approx(0.0) );
  REQUIRE( accu(abs( A44     * B44.t() - A44_times_B44_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( A44.t() * B44.t() - A44_t_times_B44_t )) == Approx(0.0) );

  REQUIRE( accu(abs( 2*A44     * B44     - two_times_A44_times_B44     )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A44.t() * B44     - two_times_A44_t_times_B44   )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A44     * B44.t() - two_times_A44_times_B44_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A44.t() * B44.t() - two_times_A44_t_times_B44_t )) == Approx(0.0) );

  REQUIRE( accu(abs( A44     * 2 * B44 -     A44_times_two_times_B44     )) == Approx(0.0) );
  REQUIRE( accu(abs( A44.t() * 2 * B44 -     A44_t_times_two_times_B44   )) == Approx(0.0) );
  REQUIRE( accu(abs( A44     * 2 * B44.t() - A44_times_two_times_B44_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( A44.t() * 2 * B44.t() - A44_t_times_two_times_B44_t )) == Approx(0.0) );

  REQUIRE( accu(abs( 2*A44     * 2*B44     - two_times_A44_times_two_times_B44     )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A44.t() * 2*B44     - two_times_A44_t_times_two_times_B44   )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A44     * 2*B44.t() - two_times_A44_times_two_times_B44_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A44.t() * 2*B44.t() - two_times_A44_t_times_two_times_B44_t )) == Approx(0.0) );


  REQUIRE( accu(abs( A44            * B44            - A44_times_B44     )) == Approx(0.0) );
  REQUIRE( accu(abs( A44.t().eval() * B44            - A44_t_times_B44   )) == Approx(0.0) );
  REQUIRE( accu(abs( A44            * B44.t().eval() - A44_times_B44_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( A44.t()        * B44.t().eval() - A44_t_times_B44_t )) == Approx(0.0) );

  REQUIRE( accu(abs( (2*A44).eval()     * B44            - two_times_A44_times_B44     )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A44.t()).eval() * B44            - two_times_A44_t_times_B44   )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A44).eval()     * B44.t().eval() - two_times_A44_times_B44_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A44.t()).eval() * B44.t().eval() - two_times_A44_t_times_B44_t )) == Approx(0.0) );

  REQUIRE( accu(abs( A44            * (2 * B44).eval()    - A44_times_two_times_B44     )) == Approx(0.0) );
  REQUIRE( accu(abs( A44.t().eval() * (2 * B44).eval()    - A44_t_times_two_times_B44   )) == Approx(0.0) );
  REQUIRE( accu(abs( A44            * (2 * B44.t()).eval() - A44_times_two_times_B44_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( A44.t().eval() * (2 * B44.t()).eval() - A44_t_times_two_times_B44_t )) == Approx(0.0) );

  REQUIRE( accu(abs( (2*A44).eval()     * (2*B44).eval()     - two_times_A44_times_two_times_B44     )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A44.t()).eval() * (2*B44).eval()     - two_times_A44_t_times_two_times_B44   )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A44).eval()     * (2*B44.t()).eval() - two_times_A44_times_two_times_B44_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A44.t()).eval() * (2*B44.t()).eval() - two_times_A44_t_times_two_times_B44_t )) == Approx(0.0) );

  }



TEST_CASE("mat_mul_real_5")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat B = fliplr(A);

  mat A55 = A( 0,0, size(5,5) );
  mat B55 = B( 0,0, size(5,5) );

  mat A55_times_B55 =
    "\
    -0.2160787   0.1649472   0.1999190  -0.1976620  -0.1252585;\
     0.1520715  -0.1467921  -0.3496545  -0.1884897  -0.0492639;\
     0.0053737  -0.0062406   0.1916407   0.2583152   0.0322787;\
    -0.3584056   0.2114322   0.2014480  -0.0361067  -0.0260243;\
    -0.0182371  -0.0207407  -0.0522859  -0.0485276   0.0678171;\
    ";

  mat A55_t_times_B55 =
    "\
     0.430191  -0.042089  -0.458772  -0.162107   0.089220;\
     0.069589   0.085881  -0.152563   0.348562   0.398588;\
    -0.080952   0.056788   0.067119   0.497201   0.348562;\
    -0.440028   0.233857   0.661467   0.067119  -0.152563;\
    -0.169075   0.251826   0.233857   0.056788   0.085881;\
    ";

  mat A55_times_B55_t =
    "\
    -0.067496   0.127397  -0.156960  -0.290754   0.194019;\
     0.147706   0.063996  -0.280946   0.315249   0.027205;\
    -0.154463  -0.099672   0.483274  -0.082829  -0.407868;\
    -0.296340   0.163712  -0.051598  -0.345899   0.025909;\
     0.190504  -0.077421  -0.389354   0.028459   0.602316;\
    ";

  mat A55_t_times_B55_t =
    "\
     0.2459910   0.1179363  -0.1064851   0.4102518  -0.2351709;\
    -0.0601617  -0.0607142   0.0417989   0.0920243   0.0569989;\
    -0.2084090  -0.1502910   0.2694883  -0.0532584  -0.0455262;\
    -0.2934704  -0.0084887   0.2831677  -0.6435349   0.0509615;\
    -0.1519108   0.0794222   0.0751652  -0.3217347   0.0492501;\
    ";

  //

  mat two_times_A55_times_B55 =
    "\
    -0.432157   0.329894   0.399838  -0.395324  -0.250517;\
     0.304143  -0.293584  -0.699309  -0.376979  -0.098528;\
     0.010747  -0.012481   0.383281   0.516630   0.064557;\
    -0.716811   0.422864   0.402896  -0.072213  -0.052049;\
    -0.036474  -0.041481  -0.104572  -0.097055   0.135634;\
    ";

  mat two_times_A55_t_times_B55 =
    "\
     0.860381  -0.084178  -0.917544  -0.324215   0.178440;\
     0.139178   0.171762  -0.305125   0.697124   0.797177;\
    -0.161904   0.113577   0.134239   0.994402   0.697124;\
    -0.880056   0.467715   1.322933   0.134239  -0.305125;\
    -0.338149   0.503651   0.467715   0.113577   0.171762;\
    ";

  mat two_times_A55_times_B55_t =
    "\
    -0.134991   0.254794  -0.313921  -0.581507   0.388038;\
     0.295412   0.127992  -0.561892   0.630497   0.054410;\
    -0.308926  -0.199343   0.966549  -0.165659  -0.815736;\
    -0.592681   0.327425  -0.103196  -0.691798   0.051819;\
     0.381007  -0.154842  -0.778709   0.056917   1.204632;\
    ";

  mat two_times_A55_t_times_B55_t =
    "\
     0.491982   0.235873  -0.212970   0.820504  -0.470342;\
    -0.120323  -0.121428   0.083598   0.184049   0.113998;\
    -0.416818  -0.300582   0.538977  -0.106517  -0.091052;\
    -0.586941  -0.016977   0.566335  -1.287070   0.101923;\
    -0.303822   0.158844   0.150330  -0.643469   0.098500;\
    ";

  //

  mat A55_times_two_times_B55 =
    "\
    -0.432157   0.329894   0.399838  -0.395324  -0.250517;\
     0.304143  -0.293584  -0.699309  -0.376979  -0.098528;\
     0.010747  -0.012481   0.383281   0.516630   0.064557;\
    -0.716811   0.422864   0.402896  -0.072213  -0.052049;\
    -0.036474  -0.041481  -0.104572  -0.097055   0.135634;\
    ";

  mat A55_t_times_two_times_B55 =
    "\
     0.860381  -0.084178  -0.917544  -0.324215   0.178440;\
     0.139178   0.171762  -0.305125   0.697124   0.797177;\
    -0.161904   0.113577   0.134239   0.994402   0.697124;\
    -0.880056   0.467715   1.322933   0.134239  -0.305125;\
    -0.338149   0.503651   0.467715   0.113577   0.171762;\
    ";

  mat A55_times_two_times_B55_t =
    "\
    -0.134991   0.254794  -0.313921  -0.581507   0.388038;\
     0.295412   0.127992  -0.561892   0.630497   0.054410;\
    -0.308926  -0.199343   0.966549  -0.165659  -0.815736;\
    -0.592681   0.327425  -0.103196  -0.691798   0.051819;\
     0.381007  -0.154842  -0.778709   0.056917   1.204632;\
    ";

  mat A55_t_times_two_times_B55_t =
    "\
     0.491982   0.235873  -0.212970   0.820504  -0.470342;\
    -0.120323  -0.121428   0.083598   0.184049   0.113998;\
    -0.416818  -0.300582   0.538977  -0.106517  -0.091052;\
    -0.586941  -0.016977   0.566335  -1.287070   0.101923;\
    -0.303822   0.158844   0.150330  -0.643469   0.098500;\
    ";

  //

  mat two_times_A55_times_two_times_B55 =
    "\
    -0.864315   0.659789   0.799676  -0.790648  -0.501034;\
     0.608286  -0.587168  -1.398618  -0.753959  -0.197056;\
     0.021495  -0.024962   0.766563   1.033261   0.129115;\
    -1.433623   0.845729   0.805792  -0.144427  -0.104097;\
    -0.072949  -0.082963  -0.209143  -0.194110   0.271268;\
    ";

  mat two_times_A55_t_times_two_times_B55 =
    "\
     1.720762508480000  -0.168355348408000  -1.835087231712000  -0.648429049028000   0.356879236660000;\
     0.278356531136000   0.343524137236000  -0.610250174464000   1.394248666196000   1.594353519068000;\
    -0.323807331272000   0.227153811280000   0.268477507864000   1.988804839396000   1.394248666196000;\
    -1.760112014908000   0.935429101824000   2.645866173324000   0.268477507864000  -0.610250174464000;\
    -0.676298185096000   1.007302825388000   0.935429101824000   0.227153811280000   0.343524137236000;\
    ";

  mat two_times_A55_times_two_times_B55_t =
    "\
    -0.269982894128000   0.509587393352000  -0.627841087236000  -1.163014609956000   0.776076770820000;\
     0.590823646192000   0.255984095800000  -1.123783487476000   1.260994352388000   0.108820335424000;\
    -0.617851781596000  -0.398686484996000   1.933097389264000  -0.331317151044000  -1.631472813896000;\
    -1.185361203228000   0.654849621340000  -0.206391610268000  -1.383596433568000   0.103637571148000;\
     0.762014443972000  -0.309683987468000  -1.557417410772000   0.113834257136000   2.409263641312000;\
    ";

  mat two_times_A55_t_times_two_times_B55_t =
    "\
     0.9839639038560000   0.4717451991920000  -0.4259405015320001   1.6410071127080001  -0.9406834141800000;\
    -0.2406466685920000  -0.2428568083480000   0.1671957402319999   0.3680971398560001   0.2279957477119999;\
    -0.8336360417759999  -0.6011639640800001   1.0779532920199999  -0.2130337897080000  -0.1821046293240000;\
    -1.1738814343720001  -0.0339548592960000   1.1326709857760002  -2.5741394118360001   0.2038460089720001;\
    -0.6076430403880000   0.3176888108440000   0.3006606227560001  -1.2869387091840001   0.1970004839200000;\
    ";

  //


  REQUIRE( accu(abs( A55     * B55     - A55_times_B55     )) == Approx(0.0) );
  REQUIRE( accu(abs( A55.t() * B55     - A55_t_times_B55   )) == Approx(0.0) );
  REQUIRE( accu(abs( A55     * B55.t() - A55_times_B55_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( A55.t() * B55.t() - A55_t_times_B55_t )) == Approx(0.0) );

  REQUIRE( accu(abs( 2*A55     * B55     - two_times_A55_times_B55     )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A55.t() * B55     - two_times_A55_t_times_B55   )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A55     * B55.t() - two_times_A55_times_B55_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A55.t() * B55.t() - two_times_A55_t_times_B55_t )) == Approx(0.0) );

  REQUIRE( accu(abs( A55     * 2 * B55 -     A55_times_two_times_B55     )) == Approx(0.0) );
  REQUIRE( accu(abs( A55.t() * 2 * B55 -     A55_t_times_two_times_B55   )) == Approx(0.0) );
  REQUIRE( accu(abs( A55     * 2 * B55.t() - A55_times_two_times_B55_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( A55.t() * 2 * B55.t() - A55_t_times_two_times_B55_t )) == Approx(0.0) );

  REQUIRE( accu(abs( 2*A55     * 2*B55     - two_times_A55_times_two_times_B55     )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A55.t() * 2*B55     - two_times_A55_t_times_two_times_B55   )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A55     * 2*B55.t() - two_times_A55_times_two_times_B55_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A55.t() * 2*B55.t() - two_times_A55_t_times_two_times_B55_t )) == Approx(0.0) );

  //

  REQUIRE( accu(abs( A55            * B55            - A55_times_B55     )) == Approx(0.0) );
  REQUIRE( accu(abs( A55.t().eval() * B55            - A55_t_times_B55   )) == Approx(0.0) );
  REQUIRE( accu(abs( A55            * B55.t().eval() - A55_times_B55_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( A55.t().eval() * B55.t().eval() - A55_t_times_B55_t )) == Approx(0.0) );

  REQUIRE( accu(abs( (2*A55).eval()     * B55            - two_times_A55_times_B55     )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A55.t()).eval() * B55            - two_times_A55_t_times_B55   )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A55).eval()     * B55.t().eval() - two_times_A55_times_B55_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A55.t()).eval() * B55.t().eval() - two_times_A55_t_times_B55_t )) == Approx(0.0) );

  REQUIRE( accu(abs( A55            * (2 * B55).eval()     - A55_times_two_times_B55     )) == Approx(0.0) );
  REQUIRE( accu(abs( A55.t().eval() * (2 * B55).eval()     - A55_t_times_two_times_B55   )) == Approx(0.0) );
  REQUIRE( accu(abs( A55            * (2 * B55.t()).eval() - A55_times_two_times_B55_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( A55.t().eval() * (2 * B55.t()).eval() - A55_t_times_two_times_B55_t )) == Approx(0.0) );

  REQUIRE( accu(abs( (2*A55).eval()     * (2*B55).eval()     - two_times_A55_times_two_times_B55     )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A55.t()).eval() * (2*B55).eval()     - two_times_A55_t_times_two_times_B55   )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A55).eval()     * (2*B55.t()).eval() - two_times_A55_times_two_times_B55_t   )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A55.t()).eval() * (2*B55.t()).eval() - two_times_A55_t_times_two_times_B55_t )) == Approx(0.0) );
  }



TEST_CASE("mat_mul_real_6")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat B = fliplr(A);

  //

  mat A_times_B_t =
    "\
    -0.064350   0.149875  -0.182277  -0.273462   0.206336;\
     0.149875   0.079491  -0.298398   0.327168   0.035695;\
    -0.182277  -0.298398   0.707103  -0.235701  -0.516759;\
    -0.273462   0.327168  -0.235701  -0.220160   0.115474;\
     0.206336   0.035695  -0.516759   0.115474   0.664298;\
    ";

  mat A_t_times_B =
    "\
     0.430191  -0.042089  -0.458772  -0.162107   0.089220   0.607990;\
     0.069589   0.085881  -0.152563   0.348562   0.398588   0.089220;\
    -0.080952   0.056788   0.067119   0.497201   0.348562  -0.162107;\
    -0.440028   0.233857   0.661467   0.067119  -0.152563  -0.458772;\
    -0.169075   0.251826   0.233857   0.056788   0.085881  -0.042089;\
     0.417147  -0.169075  -0.440028  -0.080952   0.069589   0.430191;\
    ";

  //

  mat two_times_A_times_B_t =
    "\
    -0.128699   0.299749  -0.364555  -0.546925   0.412672;\
     0.299749   0.158981  -0.596795   0.654336   0.071391;\
    -0.364555  -0.596795   1.414207  -0.471402  -1.033519;\
    -0.546925   0.654336  -0.471402  -0.440319   0.230948;\
     0.412672   0.071391  -1.033519   0.230948   1.328595;\
    ";

  mat two_times_A_t_times_B =
    "\
     0.860381  -0.084178  -0.917544  -0.324215   0.178440   1.215980;\
     0.139178   0.171762  -0.305125   0.697124   0.797177   0.178440;\
    -0.161904   0.113577   0.134239   0.994402   0.697124  -0.324215;\
    -0.880056   0.467715   1.322933   0.134239  -0.305125  -0.917544;\
    -0.338149   0.503651   0.467715   0.113577   0.171762  -0.084178;\
     0.834294  -0.338149  -0.880056  -0.161904   0.139178   0.860381;\
    ";

  //

  mat A_times_two_times_B_t =
    "\
    -0.128699   0.299749  -0.364555  -0.546925   0.412672;\
     0.299749   0.158981  -0.596795   0.654336   0.071391;\
    -0.364555  -0.596795   1.414207  -0.471402  -1.033519;\
    -0.546925   0.654336  -0.471402  -0.440319   0.230948;\
     0.412672   0.071391  -1.033519   0.230948   1.328595;\
    ";

  mat A_t_times_two_times_B =
    "\
     0.860381  -0.084178  -0.917544  -0.324215   0.178440   1.215980;\
     0.139178   0.171762  -0.305125   0.697124   0.797177   0.178440;\
    -0.161904   0.113577   0.134239   0.994402   0.697124  -0.324215;\
    -0.880056   0.467715   1.322933   0.134239  -0.305125  -0.917544;\
    -0.338149   0.503651   0.467715   0.113577   0.171762  -0.084178;\
     0.834294  -0.338149  -0.880056  -0.161904   0.139178   0.860381;\
    ";

  //

  mat two_times_A_times_two_times_B_t =
    "\
    -0.257398626992   0.599498340296  -0.729109500804  -1.093849875492   0.825343113540;\
     0.599498340296   0.317962274816  -1.193590692028   1.308671575684   0.142781030004;\
    -0.729109500804  -1.193590692028   2.828413151368  -0.942803741636  -2.067037385556;\
    -1.093849875492   1.308671575684  -0.942803741636  -0.880638524704   0.461896688368;\
     0.825343113540   0.142781030004  -2.067037385556   0.461896688368   2.657190032672;\
    ";

  mat two_times_A_t_times_two_times_B =
    "\
     1.720762508480  -0.168355348408  -1.835087231712  -0.648429049028   0.356879236660   2.431960170292;\
     0.278356531136   0.343524137236  -0.610250174464   1.394248666196   1.594353519068   0.356879236660;\
    -0.323807331272   0.227153811280   0.268477507864   1.988804839396   1.394248666196  -0.648429049028;\
    -1.760112014908   0.935429101824   2.645866173324   0.268477507864  -0.610250174464  -1.835087231712;\
    -0.676298185096   1.007302825388   0.935429101824   0.227153811280   0.343524137236  -0.168355348408;\
     1.668587103756  -0.676298185096  -1.760112014908  -0.323807331272   0.278356531136   1.720762508480;\
    ";

  //

  REQUIRE( accu(abs( A*B.t() - A_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( A.t()*B - A_t_times_B )) == Approx(0.0) );

  REQUIRE( accu(abs( 2*A*B.t()   - two_times_A_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A)*B.t() - two_times_A_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A.t()*B   - two_times_A_t_times_B )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A).t()*B - two_times_A_t_times_B )) == Approx(0.0) );

  REQUIRE( accu(abs( A*2*B.t()   - A_times_two_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( A*(2*B).t() - A_times_two_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( A.t()*2*B   - A_t_times_two_times_B )) == Approx(0.0) );
  REQUIRE( accu(abs( A.t()*(2*B) - A_t_times_two_times_B )) == Approx(0.0) );

  REQUIRE( accu(abs( 2*A*2*B.t()   - two_times_A_times_two_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A)*2*B.t() - two_times_A_times_two_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A.t()*2*B   - two_times_A_t_times_two_times_B )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A).t()*2*B - two_times_A_t_times_two_times_B )) == Approx(0.0) );

  REQUIRE( accu(abs( 2*A*(2*B).t()   - two_times_A_times_two_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A)*(2*B).t() - two_times_A_times_two_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A.t()*(2*B)   - two_times_A_t_times_two_times_B )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A).t()*(2*B) - two_times_A_t_times_two_times_B )) == Approx(0.0) );

  //

  REQUIRE( accu(abs( A*B.t().eval() - A_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( A.t().eval()*B - A_t_times_B )) == Approx(0.0) );

  REQUIRE( accu(abs( 2*A*B.t().eval()   - two_times_A_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A)*B.t().eval() - two_times_A_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A.t()).eval()*B - two_times_A_t_times_B )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A).t().eval()*B - two_times_A_t_times_B )) == Approx(0.0) );

  REQUIRE( accu(abs( A*2*(B.t().eval())        - A_times_two_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( A*(2*B).t().eval()        - A_times_two_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( A.t().eval()*2*B          - A_t_times_two_times_B )) == Approx(0.0) );
  REQUIRE( accu(abs( A.t().eval()*(2*B).eval() - A_t_times_two_times_B )) == Approx(0.0) );

  REQUIRE( accu(abs( 2*A*2*(B.t()).eval()   - two_times_A_times_two_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A)*(2*B.t()).eval() - two_times_A_times_two_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A.t().eval()*2*B     - two_times_A_t_times_two_times_B )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A).t().eval()*2*B   - two_times_A_t_times_two_times_B )) == Approx(0.0) );

  REQUIRE( accu(abs( 2*A*(2*B).t().eval()   - two_times_A_times_two_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A)*(2*B).t().eval() - two_times_A_times_two_times_B_t )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*A.t().eval()*(2*B)   - two_times_A_t_times_two_times_B )) == Approx(0.0) );
  REQUIRE( accu(abs( (2*A).t().eval()*(2*B) - two_times_A_t_times_two_times_B )) == Approx(0.0) );
  }
