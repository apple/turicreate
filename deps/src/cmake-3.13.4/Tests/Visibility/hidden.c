int hidden_function(void)
{
  return 0;
}

__attribute__((visibility("default"))) int not_hidden(void)
{
  return hidden_function();
}
