#include <toolkits/recsys/models/factorization_models.hpp>
#include <toolkits/recsys/models/itemcf.hpp>
#include <toolkits/recsys/models/item_content_recommender.hpp>
#include <toolkits/recsys/models/popularity.hpp>


namespace turi { namespace recsys {

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(recsys_factorization_model)
REGISTER_CLASS(recsys_ranking_factorization_model)
REGISTER_CLASS(recsys_itemcf)
REGISTER_CLASS(recsys_item_content_recommender)
REGISTER_CLASS(recsys_popularity)
END_CLASS_REGISTRATION







}}
