<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:output method="text" encoding="UTF-8"/>

  <xsl:variable name="api" select="document('libxml2-api.xml')"/>

  <xsl:template match="/">
    <xsl:text>#
# Officially exported symbols, for which header
# file definitions are installed in /usr/include/libxml2
#
# Automatically generated from symbols.xml and syms.xsl
#
# Versions here are *fixed* to match the libxml2 version
# at which the symbol was introduced. This ensures that
# a new client app requiring symbol foo() can't accidentally
# run with old libxml2.so not providing foo() - the global
# soname version info can't enforce this since we never
# change the soname
#

</xsl:text>
    <xsl:apply-templates select="/symbols/release"/>
  </xsl:template>

  <xsl:template match="release">
    <xsl:variable name="prev"
                  select="preceding-sibling::release[position()=1]"/>
    <xsl:text>LIBXML2_</xsl:text>
    <xsl:value-of select="string(@version)"/>
    <xsl:text> {
    global:
</xsl:text>
    <xsl:for-each select="symbol">
      <xsl:if test="string(preceding-sibling::symbol[position()=1]/@file) != string(@file)">
        <xsl:text>
# </xsl:text>
        <xsl:value-of select="@file"/>
        <xsl:text>
</xsl:text>
      </xsl:if>

      <xsl:apply-templates select="."/>
    </xsl:for-each>

    <xsl:text>} </xsl:text>
    <xsl:if test="$prev">
      <xsl:text>LIBXML2_</xsl:text>
      <xsl:value-of select="$prev/@version"/>
    </xsl:if>
    <xsl:text>;

</xsl:text>
  </xsl:template>

  <xsl:template match="symbol">
    <xsl:variable name="name" select="string(.)"/>
    <xsl:variable name="file" select="string(@file)"/>
    <xsl:choose>
      <xsl:when test="@removed">
        <xsl:text># </xsl:text>
        <xsl:value-of select="$name"/>
        <xsl:text>; removed in </xsl:text>
        <xsl:value-of select="@removed"/>
        <xsl:text>
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <!-- make sure we can find that symbol exported from the API list -->
        <xsl:variable name="def"
            select="$api/api/files/file[@name = $file]/exports[@symbol = $name]"/>
        <xsl:if test="string($def/@symbol) != $name">
          <xsl:message terminate="yes">
            <xsl:text>Failed to find definition in libxml2-api.xml:</xsl:text>
            <xsl:value-of select="$name"/>
          </xsl:message>
        </xsl:if>

        <xsl:text>  </xsl:text>
        <xsl:value-of select="$name"/>
        <xsl:text>;</xsl:text>
        <xsl:if test="$def/@type = 'variable'">
          <xsl:text> # variable</xsl:text>
        </xsl:if>
        <xsl:if test="@comment">
          <xsl:text># </xsl:text>
          <xsl:value-of select="@comment"/>
          <xsl:text>
</xsl:text>
        </xsl:if>
        <xsl:text>
</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
