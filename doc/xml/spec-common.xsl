<?xml version='1.0'?>
<xsl:stylesheet  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"  version="1.0">
<!--  elements of customization for Xiph.org specs
      common to all docbook output formats

      this file is included by the format-specific stylesheets
      $Id: spec-common.xsl,v 1.1 2002/10/26 13:22:30 giles Exp $
-->

  <xsl:param name="use.svg" select="'0'"/>

  <xsl:param name="section.autolabel" select="'1'"/>
  <xsl:param name="section.label.includes.component.label" select="'1'"/>
  <xsl:param name="appendix.autolabel" select="'1'"/>

<!-- end common elements -->
</xsl:stylesheet>
