# -*- encoding: utf-8 -*-
"""
==========
Regression
==========
"""

from pprint import pprint
import numpy as np

import sklearn.datasets
import sklearn.metrics

import autosklearn.regression
import matplotlib.pyplot as plt


############################
# Data Loading
# ============


def reg(path):
    # 读入数据
    data = np.loadtxt(path, delimiter=",", dtype=np.float32, skiprows=1)

    # print(data)
    # 切片，分离数据，x,Y
    X = data[:, 1:]
    y = data[:, 0]

    X_train, X_test, y_train, y_test = sklearn.model_selection.train_test_split(
        X, y, random_state=1
    )

    ###########################
    # Build and fit a regressor
    # =========================

    automl = autosklearn.regression.AutoSklearnRegressor(
        time_left_for_this_task=60 * 10,  # second
        per_run_time_limit=30,  # second
        tmp_folder="auto_sklearn_tmp",
        n_jobs=16,
        delete_tmp_folder_after_terminate=False,
        ensemble_kwargs={"ensemble_size": 5},
    )
    automl.fit(X_train, y_train, dataset_name="ns3-sim")

    ############################################################################
    # View the models found by auto-sklearn
    # =====================================

    print(automl.leaderboard(top_k=10))

    ######################################################
    # Print the final ensemble constructed by auto-sklearn
    # ====================================================

    pprint(automl.show_models(), indent=4)

    print(automl.sprint_statistics())

    #####################################
    # Get the Score of the final ensemble
    # ===================================
    # After training the estimator, we can now quantify the goodness of fit. One possibility for
    # is the `R2 score <https://scikit-learn.org/stable/modules/model_evaluation.html#r2-score>`_.
    # The values range between -inf and 1 with 1 being the best possible value. A dummy estimator
    # predicting the data mean has an R2 score of 0.

    train_predictions = automl.predict(X_train)
    print("Train R2 score:", sklearn.metrics.r2_score(y_train, train_predictions))
    test_predictions = automl.predict(X_test)
    print("Test R2 score:", sklearn.metrics.r2_score(y_test, test_predictions))

    ######################
    # Plot the predictions
    # ====================
    # Furthermore, we can now visually inspect the predictions. We plot the true value against the
    # predictions and show results on train and test data. Points on the diagonal depict perfect
    # predictions. Points below the diagonal were overestimated by the model (predicted value is higher
    # than the true value), points above the diagonal were underestimated (predicted value is lower than
    # the true value).

    # plt.scatter(train_predictions, y_train, label="Train samples", c="#d95f02")
    # plt.scatter(test_predictions, y_test, label="Test samples", c="#7570b3")
    # plt.xlabel("Predicted value")
    # plt.ylabel("True value")
    # plt.legend()
    # plt.plot([30, 400], [30, 400], c="k", zorder=0)
    # plt.xlim([30, 400])
    # plt.ylim([30, 400])
    # plt.tight_layout()
    # plt.savefig("regression.svg")
    # plt.show()


def main():
    path = "RegData-latest.csv"
    reg(path=path)


if __name__ == "__main__":
    main()
