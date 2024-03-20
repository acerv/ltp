# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import subprocess

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Linux Test Project'
copyright = '2024, Linux Test Project'
author = 'Linux Test Project'
release = '1.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

# Doxygen support
extensions = ['breathe']
breathe_default_project = "ltp"
breathe_projects = {'ltp': 'xml'}

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

def generate_doxygen_xml(app):
    """
    Generate API documentation via Doxygen if we are not
    inside a readthedocs server.
    """
    if not os.environ.get('READTHEDOCS', None):
        subprocess.call('doxygen Doxygen.in', shell=True)

def setup(app):
    app.add_css_file('custom.css')
    app.connect("builder-inited", generate_doxygen_xml)
