%global debug_package %{nil}
Name: kyrg-ko
Version: 1.0
Release:        1%{?dist}
Summary: kylin runtime guard kernel module.

License: GPL
#URL:
#Source0:

#BuildRequires:
#Requires:

%description
The kylin runtime guard kernel module.

%prep
%autosetup


%build
#%%configure
%make_build


%install
rm -rf $RPM_BUILD_ROOT
%make_install


%post
insmod /opt/kyrg/kyrg.ko

%preun
rmmod kyrg

%postun

%files
/opt/kyrg/

%changelog
* Tue Dec 27 2022 liuqi <liuqi1@kylinos.cn> 1.0-1
- First build.
