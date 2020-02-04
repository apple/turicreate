from six.moves import reduce as _reduce
import math
import turicreate as _tc


def entropy(probs):
    return _reduce(
        lambda x, y: x + (y * math.log(1 / y, 2) if y > 0 else 0), probs, 0
    ) / math.log(len(probs), 2)


def confidence(probs):
    return max(probs)


def relative_confidence(probs):
    lp = len(probs)
    return probs[lp - 1] - probs[lp - 2]


def get_confusion_matrix(extended_test, labels):
    # Init a matrix
    sf_confusion_matrix = {"label": [], "predicted_label": [], "prob_default": []}
    for target_l in labels:
        for predicted_l in labels:
            sf_confusion_matrix["label"].append(target_l)
            sf_confusion_matrix["predicted_label"].append(predicted_l)
            sf_confusion_matrix["prob_default"].append(0)

    sf_confusion_matrix = _tc.SFrame(sf_confusion_matrix)
    sf_confusion_matrix = sf_confusion_matrix.join(
        extended_test.groupby(
            ["label", "predicted_label"], {"count": _tc.aggregate.COUNT}
        ),
        how="left",
        on=["label", "predicted_label"],
    )
    sf_confusion_matrix = sf_confusion_matrix.fillna("count", 0)

    label_column = _tc.SFrame({"label": extended_test["label"]})
    predictions = extended_test["probs"]
    for i in range(0, len(labels)):
        new_test_data = label_column.add_columns(
            [
                predictions.apply(lambda probs: probs[i]),
                predictions.apply(lambda probs: labels[i]),
            ],
            ["prob", "predicted_label"],
        )
        if i == 0:
            test_longer_form = new_test_data
        else:
            test_longer_form = test_longer_form.append(new_test_data)

    if len(extended_test) == 0:
        sf_confusion_matrix = sf_confusion_matrix.rename(
            {"prob_default": "prob", "label": "target_label"}
        )
    else:
        sf_confusion_matrix = sf_confusion_matrix.join(
            test_longer_form.groupby(
                ["label", "predicted_label"], {"prob": _tc.aggregate.SUM("prob")}
            ),
            how="left",
            on=["label", "predicted_label"],
        )
        sf_confusion_matrix = sf_confusion_matrix.rename(
            {"label": "target_label"}
        ).fillna("prob", 0)

    def wo_divide_by_zero(a, b):
        if b == 0:
            return None
        else:
            return a * 1.0 / b

    sf_confusion_matrix["norm_prob"] = sf_confusion_matrix.join(
        sf_confusion_matrix.groupby(
            "target_label", {"sum_prob": _tc.aggregate.SUM("prob")}
        ),
        how="left",
    ).apply(lambda x: wo_divide_by_zero(x["prob"], x["sum_prob"]))
    return sf_confusion_matrix.fillna("norm_prob", 0)


def hclusterSort(vectors, dist_fn):
    distances = []
    vecs = list(vectors)[:]
    for i in range(0, len(vecs)):
        for j in range(i + 1, len(vecs)):
            distances.append(
                {"from": vecs[i], "to": vecs[j], "dist": dist_fn(vecs[i], vecs[j])}
            )
    distances = sorted(distances, key=lambda d: d["dist"])
    excluding_names = []

    while len(distances) > 0:
        min_dist = distances[0]

        new_vec = {
            "name": str(min_dist["from"]["name"]) + "|" + str(min_dist["to"]["name"]),
            "members": min_dist["from"].get("members", [min_dist["from"]])
            + min_dist["to"].get("members", [min_dist["to"]]),
        }

        excluding_names = [min_dist["from"]["name"], min_dist["to"]["name"]]

        vecs = list(filter(lambda v: v["name"] not in excluding_names, vecs))
        distances = list(
            filter(
                lambda dist: (dist["from"]["name"] not in excluding_names)
                and (dist["to"]["name"] not in excluding_names),
                distances,
            )
        )

        for v in vecs:
            total = 0
            for vi in v.get("members", [v]):
                for vj in new_vec["members"]:
                    total += dist_fn(vi, vj)
            distances.append(
                {
                    "from": v,
                    "to": new_vec,
                    "dist": total
                    / len(v.get("members", [v]))
                    / len(new_vec["members"]),
                }
            )

        vecs.append(new_vec)
        distances = sorted(distances, key=lambda d: d["dist"])

    return vecs


def l2Dist(v1, v2):
    dist = 0
    for i in range(0, len(v1["pos"])):
        dist += math.pow(v1["pos"][i] - v2["pos"][i], 2)
    return math.pow(dist, 0.5)
