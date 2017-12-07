#include <dmlc/logging.h>
#include <nnvm/op.h>
#include <utility>

NNVM_REGISTER_OP(add)
.describe("add two data together")
.set_num_inputs(2)
.set_attr("inplace_pair", std::make_pair(0, 0));

NNVM_REGISTER_OP(add)
.set_attr<std::string>("nick_name", "plus");

int main(int argc, char ** argv) {
  using namespace nnvm;
  auto add = Op::Get("add");
  auto nick = Op::GetAttr<std::string>("nick_name");

  CHECK_EQ(nick[add], "plus");
}
