Debugging in Windows

netsh dumps in ETW (Event Tracing for Windows) format
Doesn't seem to be a reader available for it (
Microsoft Network Monitor deprecated
Replaced by [Microsoft Network Analyzer](https://en.wikipedia.org/wiki/Microsoft_Network_Monitor), also deprecated
But seems everything can be done with built-in [`netsh`](https://learn.microsoft.com/en-us/windows/win32/ndf/using-netsh-to-manage-traces)
and [here](https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-R2-and-2012/jj129382)
and [here](https://learn.microsoft.com/en-us/windows-server/networking/technologies/netsh/netsh)

```
netsh trace start scenario=InternetClient
netsh trace stop
netsh trace convert input=nettrace.etl output=nettrace.txt dump=TXT
```

In fact, I see current references to using ETL with Windows Performance Analyzer (WPA)

Look for
=====Send Headers=========================
=====Receive Headers======================



Replicating Using CURL

get principal resource path
curl -X PROPFIND --silent --user 'username:password' --header 'Depth: 0' --data '@get-principal-path.xml' context-url | xmllint --format -

get calendar home set pathj
curl -X PROPFIND --silent --user 'username:password' --header 'Depth: 0' --data '@get-calendar-home-set.xml' principal-resource-url | xmllint --format -



Functional style

objective: don't construct large data structures (XML tree) buffer (HTTP response) that aren't needed

there appear to be two different objects: the POD data structures and RAII resources



Returning results through argument functors
Basically don't ever need a function result
Closely parallels asynchronous programming style
Accidentally solves the problem of not being able to overload functions with different result types


Wide Characters and Windows
I'm taking the approach that you shouldn't fight the platform.
If Windows runs on UTF-16, let it be so; if the Internet runs on (say) UTF-8, that's fine too.
The question then is what character set to use to write the application (everything in between).



Templates
I think it's a mistake that template definitions should always be in the header;
so I try as much as possible to use explicit instantiation.  I actually appreciate being able to see
the specific different instantiations that would otherwise happen implicitly.  Compile times are
not increased by exposing implementation into header files.

Template programming (not really even "template metaprogramming") is truly a different kind of C++;
it doesn't really even look the same as standard C++.  So it is good to ask what 'mode' you're in
and whether the significant extra investment in the template style is justified.

I think SFINAE causes what I would normally expect to be compile-time errors to become *not* errors.
For example, I define an overload for operator<< but which has a template instantation error.
This causes the overload to silently not compile, and now the code reverts to a less-specific
unintended overload.  Not sure how else to guard against such errors other than unit testing.


Action-Driven Parser
It is a lot of work.  I'm not sure I would do it again, even though it's potentially more efficient.


One the one hand, hard drive as much as possible to avoid copying; but then having to to UTF-16 to UTF-8
conversion seems to negate a lot of that.  UTF-8 on the HTTP boundary is unavoidable; UTF-16 in Windows APIs
is as well.


snprintf style of string construction
avoids heap allocation
allows complex string construction