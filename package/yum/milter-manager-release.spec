Summary: milter manager release files
Name: milter-manager-release
Version: 1.1.0
Release: 0
License: GPLv3+
URL: http://milter-manager.sourceforge.net/
Source: milter-manager-release.tar.gz
Group: System Environment/Base
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-%(%{__id_u} -n)
BuildArchitectures: noarch
Obsoletes: milter-manager-repository < %{version}-%{release}

%description
milter manager release files

%prep
%setup -c

%build

%install
%{__rm} -rf %{buildroot}

%{__install} -Dp -m0644 RPM-GPG-KEY-milter-manager %{buildroot}%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-milter-manager

%{__install} -Dp -m0644 milter-manager.repo %{buildroot}%{_sysconfdir}/yum.repos.d/milter-manager.repo

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, 0755)
%doc *
%pubkey RPM-GPG-KEY-milter-manager
%dir %{_sysconfdir}/yum.repos.d/
%config(noreplace) %{_sysconfdir}/yum.repos.d/milter-manager.repo
%dir %{_sysconfdir}/pki/rpm-gpg/
%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-milter-manager

%changelog
* Thu Aug 23 2012 Kenji Okimoto <okimoto@clear-code.com> - 1.1.0-0
- Rename to milter-manager-release from milter-manager-repository to follow
  convention such as centos-release and fedora-release.
* Fri Nov 11 2011 Kenji Okimoto <okimoto@clear-code.com> - 1.0.1-0
- Update GPG Key.

* Wed Aug 31 2011 Kouhei Sutou <kou@clear-code.com> - 1.0.0-2
- Use File Release System instead of Web space on SF.net.

* Wed Jan 12 2011 Kouhei Sutou <kou@clear-code.com> - 1.0.0-1
- Do not install GPG key for RPM by hand.

* Sat Feb 06 2010 Kouhei Sutou <kou@clear-code.com>
- (1.0.0-0)
- Initial package.
