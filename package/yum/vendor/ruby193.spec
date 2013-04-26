%define rubyver         1.9.3
%define rubyminorver    p392

Name:           ruby1.9
Version:        %{rubyver}%{rubyminorver}
Release:        2%{?dist}
License:        Ruby License/GPL - see COPYING
URL:            http://www.ruby-lang.org/
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:  readline readline-devel ncurses ncurses-devel gdbm gdbm-devel glibc-devel tcl-devel gcc unzip openssl-devel db4-devel byacc make libyaml libyaml-devel libffi libffi-devel
Source0:        ftp://ftp.ruby-lang.org/pub/ruby/ruby-%{rubyver}-%{rubyminorver}.tar.gz
Summary:        An interpreter of object-oriented scripting language
Group:          Development/Languages
Provides: ruby(abi) = 1.9

%description
Ruby is the interpreted scripting language for quick and easy
object-oriented programming.  It has many features to process text
files and to do system management tasks (as in Perl).  It is simple,
straight-forward, and extensible.

%prep
%setup -n ruby-%{rubyver}-%{rubyminorver}

%build
export CFLAGS="$RPM_OPT_FLAGS -Wall -fno-strict-aliasing"

%configure \
  --enable-shared \
  --disable-rpath \
  --without-X11 \
  --without-tk \
  --includedir=%{_includedir}/ruby \
  --libdir=%{_libdir} \
  --with-soname=ruby-1.9 \
  --program-suffix=1.9

make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
# installing binaries ...
make install DESTDIR=$RPM_BUILD_ROOT

#we don't want to keep the src directory
rm -rf $RPM_BUILD_ROOT/usr/src

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%{_bindir}
%{_includedir}
%{_datadir}
%{_libdir}

%changelog
* Fri Apr 26 2013 Kenji Okimoto <okimoto@clear-code.com> -1.9.3-p392
- Co-exist multiple ruby versions.

* Sun Feb 24 2013 Masahito Yoshida <masahito@axsh.net> - 1.9.3-p392
- Update ruby version to 1.9.3-p392

* Tue Jan 29 2013 Carlos Villela <cv@lixo.org> - 1.9.3-p374
- Update ruby version to 1.9.3-p374

* Tue Jan 15 2013 Carlos Villela <cv@lixo.org> - 1.9.3-p362
- Update ruby version to 1.9.3-p362

* Thu Nov 15 2012 Rajat Vig <rajat.vig@gmail.com> - 1.9.3-p327
- Update ruby version to 1.9.3-p327

* Mon Oct 22 2012 Carlos Villela <cv@lixo.org> - 1.9.3-p286
- Update ruby version to 1.9.3-p286

* Wed Jul 4 2012 Carlos Villela <cv@lixo.org> - 1.9.3-p194
- Update ruby version to 1.9.3-p194

* Wed Jan 18 2012 Mandi Walls <mandi.walls@gmail.com> - 1.9.3-p0
- Update ruby version to 1.9.3-p0

* Mon Aug 29 2011 Gregory Graf <graf.gregory@gmail.com> - 1.9.2-p290
- Update ruby version to 1.9.2-p290

* Sat Jun 25 2011 Ian Meyer <ianmmeyer@gmail.com> - 1.9.2-p180-2
- Remove non-existant --sitearchdir and --vedorarchdir from %configure
- Replace --sitedir --vendordir with simpler --libdir
- Change %{_prefix}/share to %{_datadir}

* Tue Mar 7 2011 Robert Duncan <robert@robduncan.co.uk> - 1.9.2-p180-1
- Update prerequisites to include make
- Update ruby version to 1.9.2-p180
- Install /usr/share documentation
- (Hopefully!?) platform agnostic

* Sun Jan 2 2011 Ian Meyer <ianmmeyer@gmail.com> - 1.9.2-p136-1
- Initial spec to replace system ruby with 1.9.2-p136
