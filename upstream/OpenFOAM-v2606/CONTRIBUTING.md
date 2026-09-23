# Contributions to OpenFOAM&reg;

Many thanks for considering and taking the time to contribute
to OpenFOAM&reg;, the academia- and industry-leading, forever-open-source
general-purpose fluid simulation software!

The following is a set of guidelines for contributing to OpenFOAM&reg;, which
is maintained and hosted by [Keysight Technologies](https://www.keysight.com)
on GitLab. Please use the guidelines with your best judgment, and feel free to
propose changes to this document in a [merge request](https://docs.gitlab.com/user/project/merge_requests/).

[TOC]

## How to contribute

### Reporting bugs

> Please do not use the issue tracker for usage questions such as
"How do I set up this case?" or "Why is my solution diverging?". For usage
questions, please consider using community forums and user groups.

Following these guidelines helps the maintainers and the OpenFOAM&reg; community
understand your bug ticket, reproduce the issue, and facilitate its resolution.

Before creating your bug ticket, please check [the existing list of bugs](https://gitlab.com/openfoam/core/openfoam/-/issues?sort=created_date&state=all&first_page_size=20&label_name=bug).

To create a bug ticket:

- Go to [`Issues`](https://gitlab.com/openfoam/core/openfoam/-/issues).
- Click [`New item`](https://docs.gitlab.com/user/project/issues/create_issues/) button on the top right panel.
- Select the `bug` template from the `Description` panel.
- Fill out the required template with as many details as possible, the
information it asks for helps the maintainers and the OpenFOAM&reg; community
resolve the issue faster.
- Hit the `Create issue` button on the bottom right panel.

Please note that the majority of activities of maintainers on the submitted
issues are **voluntary**; therefore, considerable delays in responses may
occur.

### Resolving existing bugs

Maintainers are not experts of everything, but the OpenFOAM&reg; community is.
Please consider resolving [the existing bugs](https://gitlab.com/openfoam/core/openfoam/-/issues).

To this end, please action as follows:

- Choose an issue to work on.
- Comment on the issue to let others know you are working on it to prevent
  duplicate effort.
- Reproduce the issue.
  - If you can't reproduce the issue or find a simpler way to reproduce, comment
    on the issue. If you can, comment to confirm, noting your OpenFOAM&reg;
    environment if it's different.
- Isolate the culprit as narrowly as possible, and report any progress.
- If possible, determine the minimal and most effective code change required to
  correct the behaviour without introducing new issues and breaking the
  backward compatibility.

### Finding new bugs

We encourage all OpenFOAM&reg; users to stress test and break any OpenFOAM&reg;
functionality by pushing OpenFOAM&reg; a little outside the "happy zone" while keeping runs reproducible.

When you hit a crash, hang, or clearly wrong physical/numerical behaviour:

- Reduce it to a minimal reproducible example and capture the exact command
  line plus the relevant log output.
- Report it via the OpenFOAM&reg; issue tracker by following the section:
  [Reporting bugs](#reporting-bugs).

### Reviewing existing merge requests

You can also improve OpenFOAM&reg; quality by reviewing existing merge requests.
You can browse open merge requests at
[Merge requests](https://gitlab.com/openfoam/core/openfoam/-/merge_requests).

When reviewing a merge request, try to stress test the changes and report
anything suspicious directly in the merge-request discussion.

> For conceptual or learning questions, such as "how does this code work?",
please prefer using community forums instead of merge request discussions.

If the problem needs broader tracking, report it via the OpenFOAM&reg; issue
tracker by following the section: [Reporting bugs](#reporting-bugs), and link
the issue in the merge-request discussion.

### Contributing to the OpenFOAM&reg; code

To add your contributions, either bug fixes or enhancements or brand-new
functionalities, you can follow the terminal-based steps below by substituting
`<>`-enclosed texts, **or adopting any other suitable method that differs from
the list below**, e.g. using
[GitLab UI based steps](https://docs.gitlab.com/user/project/repository/branches/).

If your contributions are trivial to change and test, e.g. when you want to
change `XXX` to `YYY` on line `ZZZ` of some file, please open a new issue
and make the change request. Otherwise, please follow the suggested workflow
below:

- [Fork OpenFOAM&reg; repository](https://docs.gitlab.com/user/project/repository/forking_workflow/).
  - Go to [OpenFOAM&reg; repository](https://gitlab.com/openfoam/core/openfoam).
  - Click `Fork` button on the top right panel.
  - Fill out the `Fork project` form.
  - Click `Fork project` button at the bottom panel.
- Clone the forked repository using the terminal (`git` needs to be available - see [basic git operations](https://docs.gitlab.com/topics/git/basics/)):

```bash
git clone https://gitlab.com/<your-user-name>/<your-fork-repo-name>.git <your-local-dir-name>
```

- [Create a feature branch and add your contributions to the branch](https://docs.gitlab.com/user/project/repository/branches/#create-a-branch):

```bash
cd <your-local-dir-name>
git switch -c <feature-branch-name>
git add -f <contribution-content>
git commit -m "ENH: <context-name>: <commit-message>"
```
- Push your feature branch to GitLab:
```bash
git push --set-upstream origin <feature-branch-name>
```
- [Open a merge request targeting the `develop` branch in the OpenFOAM&reg; repository](https://docs.gitlab.com/user/project/merge_requests/creating_merge_requests/).
- Select the `community-contributions` template from the `Description` panel.
- Fill out the template with as many details as possible.
- Click `Submit merge request` button.
  - OpenFOAM&reg; maintainers will review your merge request, and provide
  feedback and ask questions. Note that the review is mostly *voluntary*; therefore,
  please expect considerable delays in responses. Also, note that the merge
  is not guaranteed.
  - Following the OpenFOAM&reg; maintainers' review, please update your
  merge-request branch by adding new commits prefixed with the `SQUASH` tag.
  Do not force-push or rewrite the branch history.
  - Finally, after the approval of the merge request by the maintainers,
  you squash the commits prefixed with `SQUASH` into the primary commits.
- If you don't intend to make more contributions in the future after your work is
merged, you can [unlink your fork](https://docs.gitlab.com/user/project/repository/forking_workflow/#unlink-a-fork) from its upstream repository or delete your fork.

### Licensing and copyright

- OpenFOAM&reg; is distributed under the GNU General Public License version 3
(GPLv3), see [LICENSE](./LICENSE.md) for the details.

## Style guide

### Git commit messages

#### Commit title

- In the title, start your commit message with one of the tags below - all in
capitals:
    - `BUG`: Bug fixes for reported issues.
    - `DOC`: New or improved code documentation.
    - `COMP`: New or improved code related to compiler behaviour and compilation processes.
    - `CONFIG`: Code related to software configuration, e.g. `config.sh`.
    - `DEFEATURE`: Code or functionalities that need to be deprecated/removed.
    - `REVERT`: Commits that need to be reverted.
    - `SUBMODULE`: Code related to the [`modules`](https://gitlab.com/openfoam/core/openfoam/-/tree/master/modules) or [`plugins`](https://gitlab.com/openfoam/core/openfoam/-/tree/master/plugins).
    - `STYLE`: New or improved style changes, e.g. removing trailing blank lines.
    - `TUT`: Any changes to the [`tutorials`](https://gitlab.com/openfoam/core/openfoam/-/tree/master/tutorials).
    - `ENH`: New or improved code for the remaining context.
- Optionally, provide the context of the commit message. For example, if the
changes are related to the [`functionObjects`](https://gitlab.com/openfoam/core/openfoam/-/tree/master/src/functionObjects), continue the commit as such:
```git
ENH: functionObjects: ...
```
- Following `:`, use lowercase letters unless a specific word should be used.
- Use imperative clauses: e.g. use `add func ...` instead of `adds func ...` or `added func ...`.
- Limit the title message to 72 characters, preferably around 50 characters.
- If applicable, crosslink the commit with the relevant issue after the title:
e.g. `... (fixes #1234)` or `... (#1234)` - see [GitLab Doc](https://docs.gitlab.com/user/project/issues/crosslinking_issues/#from-commit-messages) for the details.
- No end punctuation.

#### Commit body

- Optionally, you do not need to add more text if the title is sufficient.
  - If still necessary, explain what changed and why.
  - If applicable, note what needs to be changed further.
- Always leave the second line blank.
- Wrap the body message to 72 characters.
- Use imperative clauses: e.g. use `add func ...` instead of `adds func ...` or `added func ...`.
- Use hyphens for the bullet points, if needed, e.g.:
```
<tag>: <context>: <message>

- <bullet point 1>
- <bullet point 2> etc.
```

#### Commit authorship

- You can use your real name or pseudonyms.
- You do not need to use your actual email address.
- If the commit belongs to multiple developers, consider adding the co-authors
at the end of the commit body, e.g.:
```git
Co-authored-by: name <additional-dev-1@example.com>
Co-authored-by: name <additional-dev-2@example.com>
```

### Code guidelines

Please follow the following evolving style guides for various aspects of the
OpenFOAM&reg; code:

- [Code development](https://gitlab.com/openfoam/core/openfoam/-/wikis/coding/git-workflow):
Suggestions for git-based workflows.
- [Coding patterns](https://gitlab.com/openfoam/core/openfoam/-/wikis/coding/patterns/patterns):
Frequently encountered OpenFOAM&reg; idioms.
- [File naming](https://gitlab.com/openfoam/core/openfoam/-/wikis/coding/style/filenames):
Naming conventions used in OpenFOAM&reg;.
- [Coding style](https://gitlab.com/openfoam/core/openfoam/-/wikis/coding/style/style): Coding style in OpenFOAM&reg;.
- [Coding scripts](https://gitlab.com/openfoam/core/openfoam/-/wikis/coding/scripts/scripts): General coding scripts for OpenFOAM&reg;.

## Code of conduct

We encourage a positive and collaborative environment.
Please adhere to our [Code of Conduct](./CODE_OF_CONDUCT.md)
when participating in this repository.

## Contact

Please do not hesitate to contact us via the
[Contact Us](https://www.openfoam.com/contact-us) online form
if you have any feedback outside of the contribution scope.

## Acknowledgements

We appreciate the efforts of the entire community of OpenFOAM&reg; users in
[contributing](./CONTRIBUTORS.md) to the improvement and evolution of this
powerful CFD tool.

Thank you again for your interest and contributions to the OpenFOAM&reg;
software!

<!-- Copyright 2026 Keysight Technologies -->

<!----------------------------------------------------------------------------->
