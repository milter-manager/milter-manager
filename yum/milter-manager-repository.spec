Summary: milter manager RPM repository configuration
Name: milter-manager-repository
Version: 1.0.0
Release: 0
License: GPLv3+
URL: http://milter-manager.sourceforge.net/
Source: milter-manager-repository.tar.gz
Group: System Environment/Base
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-%(%{__id_u} -n)
BuildArchitectures: noarch

%description
milter manager RPM repository configuration.

%prep
%setup -c

%build

%install
%{__rm} -rf %{buildroot}

%{__install} -Dp -m0644 RPM-GPG-KEY-milter-manager %{buildroot}%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-milter-manager

%{__install} -Dp -m0644 milter-manager.repo %{buildroot}%{_sysconfdir}/yum.repos.d/milter-manager.repo

%clean
%{__rm} -rf %{buildroot}

%post
rpm -q gpg-pubkey-1c837f31-4a2b9c3f &>/dev/null || \
    rpm --import %{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-milter-manager

%files
%defattr(-, root, root, 0755)
%doc *
%pubkey RPM-GPG-KEY-milter-manager
%dir %{_sysconfdir}/yum.repos.d/
%config(noreplace) %{_sysconfdir}/yum.repos.d/milter-manager.repo
%dir %{_sysconfdir}/pki/rpm-gpg/
%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-milter-manager

%changelog
* Sat Feb 06 2010 Kouhei Sutou <kou@clear-code.com>
- (1.0.0-0)
- Initial package.
