Summary:	Prun job serialization library
Name:		prun-elliptics
Version:	0.1
Release:	1%{?dist}.1

License:	Apache License, Version 2.0
Group:		System Environment/Libraries
URL:		https://github.com/abudnik/prun-elliptics
Source0:	%{name}-%{version}.tar.bz2
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:	cmake
BuildRequires:	boost-devel
BuildRequires:  elliptics-client-devel

%description
Prun-elliptics is a prun job serialization library


%package prun-elliptics
Summary:	Prun job serialization library
Group:		System Environment/Libraries

%description prun-elliptics
Prun-elliptics is a prun job serialization library


%prep
%setup -q

%build
export DESTDIR="%{buildroot}"
%{cmake} .

make


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
rm -f %{buildroot}%{_libdir}/*.a
rm -f %{buildroot}%{_libdir}/*.la


%clean
rm -rf %{buildroot}


%files prun-elliptics
%defattr(-,root,root,-)
%{_libdir}/lib*.so


%changelog
* Tue Aug 18 2015 Andrey Budnik <budnik27@gmail.com> - 0.1
- Initial release.
