from distutils.core import setup, Extension

module = Extension('classic', sources = ['classicModule.cpp', 'class.cpp'])

setup (name = 'PackageName',
       version = '1.0',
       description = 'A Python interface to the CLASSIC data container',
       author = "Michael Olberg",
       author_email = "michael.olberg@chalmers.se",
       maintainer = "Michael Olberg",
       maintainer_email = "michael.olberg@chalmers.se",
       keywords = ["C-extension","Fibonacci"],
       ext_modules = [module])
