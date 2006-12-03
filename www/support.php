<?php $selected="support"; include("header.inc"); ?>

<A name="mailing-lists"><h2>Mailing lists</h2></A>
There is one way to get in contact with the SvxLink "community". That is
through the mailing list. The address to the mailing list is
<A href="mailto:svxlink-devel@lists.sourceforge.net">
svxlink-devel@lists.sourceforge.net</A>. So, whatever your business is, please
use the mailing list. Do not use EchoLink or direct e-mail to the author. The
list is very low volume so don't be afraid to register.
<P>
There are a couple of reasons why only the mailing list should be used for
support:
<UL>
  <LI>There are more people that can help. I only use RedHat Linux but there are
      people on the mailing list that have gotten SvxLink to work under other
      distributions.</LI>
  <LI>Documentation. All mails sent to the mailing list get stored in the
      <A href="https://sourceforge.net/mailarchive/forum.php?forum_id=34097">mailing list archive</A>.
      A good way to find solutions to problems is to
      <A href="https://sourceforge.net/search/index.php?type_of_search=mlists&group_id=84813&forum_id=34097">search</A>
      the archive.</LI>
  <LI>Other people subscribing to the mailing list might get helped by the
      discussion.</LI>
</UL>
<P>
There is also another mailing list svxlink-announce@lists.sourceforge.net, which
is even lower volume. Just a single mail per SvxLink release, which is not that
often. If you join the svxlink-devel list there is no need to join the
svxlink-announce list.
<P>
To subscribe to the mailing lists go
<A href="https://sourceforge.net/mail/?group_id=84813">here</A>.
<P>


<A name="reporting-bugs"><h2>Reporting bugs</h2></A>
Bugs should be reported to the svxlink-devel mailing list mentioned above.
It can be hard filing a good bug report. But I assure you, it is even harder to
understand a bad bug report. So, if you find a bug, take the time to go through
the steps below to make a good bug report.

<OL>
  <LI>
    Read the documentation thoroughly to verify that you have not
    misunderstood something. This includes
    <A href="install.php">the installation documentation</A>,
    <A href="svxlink_usage.php">the svxlink server docs</A> and
    <A href="qtel_usage.php">the qtel docs</A>,
    depending on what application you found the bug in.
  </LI>
  <LI>
    If possible, try to reproduce the bug. A way to reproduce the bug makes it
    much easier for the developer to find and fix the bug. If it is not possible
    to reproduce it, try to remember what happened right before the bug
    appeared. Include a detailed step by step instruction in the report of how
    to reproduce the bug.
  </LI>
  <LI>
    Write a bug report and send it off to the svxlink-devel mailing list. The
    bug report should include the following:
    <UL>
      <LI>
      	A <em>detailed</em> description of the bug. Do not just write "this
      	feature does not work". Explain in what way it does not work. There can
      	be many ways for a certain feature to fail.
      </LI>
      <LI>
        If possible, describe how to reproduce the bug.
      </LI>
    </UL>
  </LI>
</OL>

If these simple guide lines are followed, bugs will be found and fixed much
faster.

<?php include("footer.inc"); ?>

