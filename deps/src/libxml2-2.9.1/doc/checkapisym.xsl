<?xml version="1.0"?>
<!-- This stylesheet is used to check that symbols exported
     from libxml2-api.xml are also present in the symbol file
     symbols.xml which is used to generate libxml2.syms setting
     up the allowed access point to the shared libraries -->


<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:output method="text" encoding="UTF-8"/>

  <xsl:variable name="syms" select="document('symbols.xml')"/>

  <xsl:template match="/">
    <xsl:message terminate="no">
      <xsl:text>Looking for functions in symbols.xml</xsl:text>
    </xsl:message>
    <xsl:apply-templates select="/api/symbols/function"/>
    <xsl:message terminate="no">
      <xsl:text>Found </xsl:text>
      <xsl:value-of select="count(/api/symbols/function)"/>
      <xsl:text> functions</xsl:text>
    </xsl:message>
    <xsl:message terminate="no">
      <xsl:text>Looking for variables in symbols.xml</xsl:text>
    </xsl:message>
    <xsl:apply-templates select="/api/symbols/variable"/>
    <xsl:message terminate="no">
      <xsl:text>Found </xsl:text>
      <xsl:value-of select="count(/api/symbols/variable)"/>
      <xsl:text> variables</xsl:text>
    </xsl:message>
  </xsl:template>

  <xsl:template match="function|variable">
    <xsl:variable name="name" select="@name"/>
    <xsl:variable name="symbol"
        select="$syms/symbols/release/symbol[. = $name]"/>
    <xsl:if test="string($symbol) != $name">
      <xsl:message terminate="yes">
        <xsl:text>Failed to find export in symbols.xml: </xsl:text>
        <xsl:value-of select="$name"/>
      </xsl:message>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
