CMake Review Process
********************

The following documents the process for reviewing and integrating changes.
See `CONTRIBUTING.rst`_ for instructions to contribute changes.
See documentation on `CMake Development`_ for more information.

.. _`CONTRIBUTING.rst`: ../../CONTRIBUTING.rst
.. _`CMake Development`: README.rst

.. contents:: The review process consists of the following steps:

Merge Request
=============

A user initiates the review process for a change by pushing a *topic
branch* to his or her own fork of the `CMake Repository`_ on GitLab and
creating a *merge request* ("MR").  The new MR will appear on the
`CMake Merge Requests Page`_.  The rest of the review and integration
process is managed by the merge request page for the change.

During the review process, the MR submitter should address review comments
or test failures by updating the MR with a (force-)push of the topic
branch.  The update initiates a new round of review.

We recommend that users enable the "Remove source branch when merge
request is accepted" option when creating the MR or by editing it.
This will cause the MR topic branch to be automatically removed from
the user's fork during the `Merge`_ step.

.. _`CMake Merge Requests Page`: https://gitlab.kitware.com/cmake/cmake/merge_requests
.. _`CMake Repository`: https://gitlab.kitware.com/cmake/cmake

Workflow Status
---------------

`CMake GitLab Project Developers`_ may set one of the following labels
in GitLab to track the state of a MR:

* ``workflow:wip`` indicates that the MR needs additional updates from
  the MR submitter before further review.  Use this label after making
  comments that require such updates.

* ``workflow:in-review`` indicates that the MR awaits feedback from a
  human reviewer or from `Topic Testing`_.  Use this label after making
  comments requesting such feedback.

* ``workflow:nightly-testing`` indicates that the MR awaits results
  of `Integration Testing`_.  Use this label after making comments
  requesting such staging.

* ``workflow:expired`` indicates that the MR has been closed due
  to a period of inactivity.  See the `Expire`_ step.  Use this label
  after closing a MR for this reason.

The workflow status labels are intended to be mutually exclusive,
so please remove any existing workflow label when adding one.

.. _`CMake GitLab Project Developers`: https://gitlab.kitware.com/cmake/cmake/settings/members

Robot Review
============

The "Kitware Robot" (``@kwrobot``) automatically performs basic checks on
the commits proposed in a MR.  If all is well the robot silently reports
a successful "build" status to GitLab.  Otherwise the robot posts a comment
with its diagnostics.  **A topic may not be merged until the automatic
review succeeds.**

Note that the MR submitter is expected to address the robot's comments by
*rewriting* the commits named by the robot's diagnostics (e.g., via
``git rebase -i``). This is because the robot checks each commit individually,
not the topic as a whole. This is done in order to ensure that commits in the
middle of a topic do not, for example, add a giant file which is then later
removed in the topic.

Automatic Check
---------------

The automatic check is repeated whenever the topic branch is updated.
One may explicitly request a re-check by adding a comment with the
following command among the `comment trailing lines`_::

  Do: check

``@kwrobot`` will add an award emoji to the comment to indicate that it
was processed and also run its checks again.

Automatic Format
----------------

The automatic check will reject commits introducing source code not
formatted according to ``clang-format``.  One may ask the robot to
automatically rewrite the MR topic branch with expected formatting
by adding a comment with the following command among the
`comment trailing lines`_::

  Do: reformat

``@kwrobot`` will add an award emoji to the comment to indicate that it
was processed and also rewrite the MR topic branch and force-push an
updated version with every commit formatted as expected by the check.

Human Review
============

Anyone is welcome to review merge requests and make comments!

Please make comments directly on the MR page Discussion and Changes tabs
and not on individual commits.  Comments on a commit may disappear
from the MR page if the commit is rewritten in response.

Reviewers may add comments providing feedback or to acknowledge their
approval.  Lines of specific forms will be extracted during the `merge`_
step and included as trailing lines of the generated merge commit message.
Each review comment consists of up to two parts which must be specified
in the following order: `comment body`_, then `comment trailing lines`_.
Each part is optional, but they must be specified in this order.

Comment Body
------------

The body of a comment may be free-form `GitLab Flavored Markdown`_.
See GitLab documentation on `Special GitLab References`_ to add links to
things like issues, commits, or other merge requests (even across projects).

Additionally, a line in the comment body may start with one of the
following votes:

* ``-1`` or ``:-1:`` indicates "the change is not ready for integration".

* ``+1`` or ``:+1:`` indicates "I like the change".
  This adds an ``Acked-by:`` trailer to the `merge`_ commit message.

* ``+2`` indicates "the change is ready for integration".
  This adds a ``Reviewed-by:`` trailer to the `merge`_ commit message.

* ``+3`` indicates "I have tested the change and verified it works".
  This adds a ``Tested-by:`` trailer to the `merge`_ commit message.

.. _`GitLab Flavored Markdown`: https://gitlab.kitware.com/help/user/markdown.md
.. _`Special GitLab References`: https://gitlab.kitware.com/help/user/markdown.md#special-gitlab-references

Comment Trailing Lines
----------------------

Zero or more *trailing* lines in the last section of a comment may appear
with the form ``Key: Value``.  The first such line should be separated
from a preceding `comment body`_ by a blank line.  Any key-value pair(s)
may be specified for human reference.  A few specific keys have meaning to
``@kwrobot`` as follows.

Comment Trailer Votes
^^^^^^^^^^^^^^^^^^^^^

Among the `comment trailing lines`_ one may cast a vote using one of the
following pairs followed by nothing but whitespace before the end of the line:

* ``Rejected-by: me`` indicates "the change is not ready for integration".
* ``Acked-by: me`` indicates "I like the change".
  This adds an ``Acked-by:`` trailer to the `merge`_ commit message.
* ``Reviewed-by: me`` indicates "the change is ready for integration".
  This adds a ``Reviewed-by:`` trailer to the `merge`_ commit message.
* ``Tested-by: me`` indicates "I have tested the change and verified it works".
  This adds a ``Tested-by:`` trailer to the `merge`_ commit message.

Each ``me`` reference may instead be an ``@username`` reference or a full
``Real Name <user@domain>`` reference to credit someone else for performing
the review.  References to ``me`` and ``@username`` will automatically be
transformed into a real name and email address according to the user's
GitLab account profile.

Comment Trailer Commands
^^^^^^^^^^^^^^^^^^^^^^^^

Among the `comment trailing lines`_ authorized users may issue special
commands to ``@kwrobot`` using the form ``Do: ...``:

* ``Do: check`` explicitly re-runs the robot `Automatic Check`_.
* ``Do: reformat`` rewrites the MR topic for `Automatic Format`_.
* ``Do: test`` submits the MR for `Topic Testing`_.
* ``Do: stage`` submits the MR for `Integration Testing`_.
* ``Do: merge`` submits the MR for `Merge`_.

See the corresponding sections for details on permissions and options
for each command.

Topic Testing
=============

CMake has a `buildbot`_ instance watching for merge requests to test.
`CMake GitLab Project Developers`_ may activate buildbot on a MR by
adding a comment with a command among the `comment trailing lines`_::

  Do: test

``@kwrobot`` will add an award emoji to the comment to indicate that it
was processed and also inform buildbot about the request.  The buildbot
user (``@buildbot``) will schedule builds and respond with a comment
linking to the `CMake CDash Page`_ with a filter for results associated
with the topic test request.  If the MR topic branch is updated by a
push a new ``Do: test`` command is needed to activate testing again.

The ``Do: test`` command accepts the following arguments:

* ``--stop``: clear the list of commands for the merge request
* ``--clear``: clear previous commands before adding this command
* ``--regex-include <arg>`` or ``-i <arg>``: only build on builders
  matching ``<arg>`` (a Python regular expression)
* ``--regex-exclude <arg>`` or ``-e <arg>``: exclude builds on builders
  matching ``<arg>`` (a Python regular expression)

Builder names follow the pattern ``project-host-os-buildtype-generator``:

* ``project``: always ``cmake`` for CMake builds
* ``host``: the buildbot host
* ``os``: one of ``windows``, ``osx``, or ``linux``
* ``buildtype``: ``release`` or ``debug``
* ``generator``: ``ninja``, ``makefiles``, ``vs<year>``,
  or ``lint-iwyu-tidy``

The special ``lint-<tools>`` generator name is a builder that builds
CMake using lint tools but does not run the test suite (so the actual
generator does not matter).

.. _`buildbot`: http://buildbot.net
.. _`CMake CDash Page`: https://open.cdash.org/index.php?project=CMake

Integration Testing
===================

The above `topic testing`_ tests the MR topic independent of other
merge requests and on only a few key platforms and configurations.
The `CMake Testing Process`_ also has a large number of machines
provided by Kitware and generous volunteers that cover nearly all
supported platforms, generators, and configurations.  In order to
avoid overwhelming these resources, they do not test every MR
individually.  Instead, these machines follow an *integration branch*,
run tests on a nightly basis (or continuously during the day), and
post to the `CMake CDash Page`_.  Some follow ``master``.  Most follow
a special integration branch, the *topic stage*.

The topic stage is a special branch maintained by the "Kitware Robot"
(``@kwrobot``).  It consists of the head of the MR target integration
branch (e.g. ``master``) branch followed by a sequence of merges each
integrating changes from an open MR that has been staged for integration
testing.  Each time the target integration branch is updated the stage
is rebuilt automatically by merging the staged MR topics again.

`CMake GitLab Project Developers`_ may stage a MR for integration testing
by adding a comment with a command among the `comment trailing lines`_::

  Do: stage

``@kwrobot`` will add an award emoji to the comment to indicate that it
was processed and also attempt to add the MR topic branch to the topic
stage.  If the MR cannot be added (e.g. due to conflicts) the robot will
post a comment explaining what went wrong.

Once a MR has been added to the topic stage it will remain on the stage
until one of the following occurs:

* The MR topic branch is updated by a push.

* The MR target integration branch (e.g. ``master``) branch is updated
  and the MR cannot be merged into the topic stage again due to conflicts.

* A developer or the submitter posts an explicit ``Do: unstage`` command.
  This is useful to remove a MR from the topic stage when one is not ready
  to push an update to the MR topic branch.  It is unnecessary to explicitly
  unstage just before or after pushing an update because the push will cause
  the MR to be unstaged automatically.

* The MR is closed.

* The MR is merged.

Once a MR has been removed from the topic stage a new ``Do: stage``
command is needed to stage it again.

.. _`CMake Testing Process`: testing.rst

Resolve
=======

A MR may be resolved in one of the following ways.

Merge
-----

Once review has concluded that the MR topic is ready for integration,
`CMake GitLab Project Masters`_ may merge the topic by adding a comment
with a command among the `comment trailing lines`_::

  Do: merge

``@kwrobot`` will add an award emoji to the comment to indicate that it
was processed and also attempt to merge the MR topic branch to the MR
target integration branch (e.g. ``master``).  If the MR cannot be merged
(e.g. due to conflicts) the robot will post a comment explaining what
went wrong.  If the MR is merged the robot will also remove the source
branch from the user's fork if the corresponding MR option was checked.

The robot automatically constructs a merge commit message of the following
form::

  Merge topic 'mr-topic-branch-name'

  00000000 commit message subject line (one line per commit)

  Acked-by: Kitware Robot <kwrobot@kitware.com>
  Merge-request: !0000

Mention of the commit short sha1s and MR number helps GitLab link the
commits back to the merge request and indicates when they were merged.
The ``Acked-by:`` trailer shown indicates that `Robot Review`_ passed.
Additional ``Acked-by:``, ``Reviewed-by:``, and similar trailers may be
collected from `Human Review`_ comments that have been made since the
last time the MR topic branch was updated with a push.

The ``Do: merge`` command accepts the following arguments:

* ``-t <topic>``: substitute ``<topic>`` for the name of the MR topic
  branch in the constructed merge commit message.

Additionally, ``Do: merge`` extracts configuration from trailing lines
in the MR description:

* ``Topic-rename: <topic>``: substitute ``<topic>`` for the name of
  the MR topic branch in the constructed merge commit message.
  The ``-t`` option overrides this.

.. _`CMake GitLab Project Masters`: https://gitlab.kitware.com/cmake/cmake/settings/members

Close
-----

If review has concluded that the MR should not be integrated then it
may be closed through GitLab.

Expire
------

If progress on a MR has stalled for a while, it may be closed with a
``workflow:expired`` label and a comment indicating that the MR has
been closed due to inactivity.

Contributors are welcome to re-open an expired MR when they are ready
to continue work.  Please re-open *before* pushing an update to the
MR topic branch to ensure GitLab will still act on the association.
