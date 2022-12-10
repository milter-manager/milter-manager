---
tags: []
title: milter-manager is powered by packagecloud.io
---
Since Oct, 2016, milter-manager is powered by packagecloud.io.
They kindly provides us open-source plan since ever!
<!--more-->


Here is our use-case which explains in forms of interview.

### What was one quantifiable impact, and one intangible benefit that packagecloud brought?

Every new milter-manager release, we don't need to maintain own APT/YUM repository by ourself anymore.
As milter-manager is developed for a long time, the repository also contains many packages, it takes disk space a lot.
packagecloud.io provides enough disk spaces for keeping milter-manager repository.

And one more thing, we don't need to maintain signing-key by ourself.

### What other alternatives have you considered and why did you choose packagecloud?

We are considered migration to [OSDN](https://ja.osdn.net/) at that time, but OSDN didn't provide hosting APT/YUM functionality yet.

So, we decided migrating to packagecloud.io.

### What was life like before you used packagecloud?

Before migrating to packagecloud.io, we used sourceforge.net.
At that time, we maintained APT/YUM repository on sourceforge.net, but it was not reliable because they changed download URL sometimes without enough announcement in advance. We need to follow-up those changes time to time.

### What pain did packagecloud help solve (besides being free for OSS software)?

As we mentioned above, we are freed from maintainance of APT/YUM repository, so we can focus on developing milter-manager itself.

Thanks, packagecloud.io for sponsoring milter-manager!
