<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
		xmlns="http://www.devhelp.net/book"
		xmlns:exsl="http://exslt.org/common"
		xmlns:str="http://exslt.org/strings"
		extension-element-prefixes="exsl str"
		exclude-result-prefixes="exsl str">
  <!-- The stylesheet for the html pages -->
  <xsl:import href="html.xsl"/>

  <xsl:output method="xml" encoding="UTF-8" indent="yes"/>

  <!-- Build keys for all symbols -->
  <xsl:key name="symbols" match="/api/symbols/*" use="@name"/>

  <xsl:template match="/api">
    <book title="{@name} Reference Manual" link="index.html" author="" name="{@name}">
      <xsl:apply-templates select="files"/>
      <xsl:apply-templates select="symbols"/>
    </book>
    <xsl:call-template name="generate_index"/>
    <xsl:call-template name="generate_general"/>
  </xsl:template>
  <xsl:template match="/api/files">
    <chapters>
      <sub name="API" link="general.html">
        <xsl:apply-templates select="file"/>
      </sub>
    </chapters>
  </xsl:template>
  <xsl:template match="/api/files/file">
    <xsl:variable name="module" select="@name"/>
    <xsl:variable name="prev" select="string(preceding-sibling::file[position()=1]/@name)"/>
    <xsl:variable name="next" select="string(following-sibling::file[position()=1]/@name)"/>
    <sub name="{@name}" link="libxml2-{@name}.html"/>
    <xsl:document xmlns="" href="libxml2-{@name}.html" method="xml" indent="yes" encoding="UTF-8">
      <html>
        <head>
	  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>
	  <title><xsl:value-of select="concat(@name, ': ', summary)"/></title>
	  <meta name="generator" content="Libxml2 devhelp stylesheet"/>
	  <link rel="start" href="index.html" title="libxml2 Reference Manual"/>
	  <link rel="up" href="general.html" title="API"/>
	  <link rel="stylesheet" href="style.css" type="text/css"/>
	  <link rel="chapter" href="general.html" title="API"/>
        </head>
	<body bgcolor="white" text="black" link="#0000FF" vlink="#840084" alink="#0000FF">

          <table class="navigation" width="100%" summary="Navigation header" cellpadding="2" cellspacing="2">
	    <tr valign="middle">
	      <xsl:if test="$prev != ''">
		<td><a accesskey="p" href="libxml2-{$prev}.html"><img src="left.png" width="24" height="24" border="0" alt="Prev"/></a></td>
	      </xsl:if>
              <td><a accesskey="u" href="general.html"><img src="up.png" width="24" height="24" border="0" alt="Up"/></a></td>
              <td><a accesskey="h" href="index.html"><img src="home.png" width="24" height="24" border="0" alt="Home"/></a></td>
	      <xsl:if test="$next != ''">
		<td><a accesskey="n" href="libxml2-{$next}.html"><img src="right.png" width="24" height="24" border="0" alt="Next"/></a></td>
	      </xsl:if>
              <th width="100%" align="center">libxml2 Reference Manual</th>
            </tr>
	  </table>
	  <h2><span class="refentrytitle"><xsl:value-of select="@name"/></span></h2>
	  <p><xsl:value-of select="@name"/> - <xsl:value-of select="summary"/></p>
	  <p><xsl:value-of select="description"/></p>
	  <xsl:if test="deprecated">
	    <p> WARNING: this module is deprecated !</p>
	  </xsl:if>
	  <p>Author(s): <xsl:value-of select="author"/></p>
	  <div class="refsynopsisdiv">
	  <h2>Synopsis</h2>
	  <pre class="synopsis">
	    <xsl:apply-templates mode="synopsis" select="exports"/>
	  </pre>
	  </div>
	  <div class="refsect1" lang="en">
	  <h2>Description</h2>
	  </div>
	  <div class="refsect1" lang="en">
	  <h2>Details</h2>
	  <div class="refsect2" lang="en">
	    <xsl:apply-templates mode="details" select="/api/symbols/macro[@file=$module]"/>
	    <xsl:apply-templates mode="details" select="/api/symbols/typedef[@file=$module] | /api/symbols/struct[@file=$module]"/>
	    <xsl:apply-templates mode="details" select="/api/symbols/functype[@file=$module]"/>
	    <xsl:apply-templates mode="details" select="/api/symbols/variable[@file=$module]"/>
	    <xsl:apply-templates mode="details" select="/api/symbols/function[@file=$module]"/>
	  </div>
	  </div>
	</body>
      </html>
    </xsl:document>
  </xsl:template>
  <xsl:template match="/api/symbols">
    <functions>
      <xsl:apply-templates select="macro"/>
      <xsl:apply-templates select="enum"/>
      <xsl:apply-templates select="typedef"/>
      <xsl:apply-templates select="struct"/>
      <xsl:apply-templates select="functype"/>
      <xsl:apply-templates select="variable"/>
      <xsl:apply-templates select="function"/>
    </functions>
  </xsl:template>
  <xsl:template match="/api/symbols/functype">
    <function name="{@name}" link="libxml2-{@file}.html#{@name}"/>
  </xsl:template>
  <xsl:template match="/api/symbols/function">
    <function name="{@name} ()" link="libxml2-{@file}.html#{@name}"/>
  </xsl:template>
  <xsl:template match="/api/symbols/typedef">
    <function name="{@name}" link="libxml2-{@file}.html#{@name}"/>
  </xsl:template>
  <xsl:template match="/api/symbols/enum">
    <function name="{@name}" link="libxml2-{@file}.html#{@name}"/>
  </xsl:template>
  <xsl:template match="/api/symbols/struct">
    <function name="{@name}" link="libxml2-{@file}.html#{@name}"/>
  </xsl:template>
  <xsl:template match="/api/symbols/macro">
    <function name="{@name}" link="libxml2-{@file}.html#{@name}"/>
  </xsl:template>
  <xsl:template match="/api/symbols/variable">
    <function name="{@name}" link="libxml2-{@file}.html#{@name}"/>
  </xsl:template>

</xsl:stylesheet>
