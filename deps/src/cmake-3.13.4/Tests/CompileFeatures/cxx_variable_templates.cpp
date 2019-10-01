
template <typename T>
constexpr T pi = T(3.1415926535897932385);

int someFunc()
{
  auto pi_int = pi<int>;
  auto pi_float = pi<float>;
  return pi_int - 3;
}
