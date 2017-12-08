<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
		xmlns:exsl="http://exslt.org/common"
		xmlns:str="http://exslt.org/strings"
		extension-element-prefixes="exsl str"
		exclude-result-prefixes="exsl str">
  <xsl:output method="xml" encoding="UTF-8" indent="yes"/>

  <!-- This is convoluted but needed to force the current document to
       be the API one and not the result tree from the tokenize() result,
       because the keys are only defined on the main document -->
  <xsl:template mode="dumptoken" match='*'>
    <xsl:param name="token"/>
    <xsl:variable name="ref" select="key('symbols', $token)"/>
    <xsl:choose>
      <xsl:when test="$ref">
        <a href="libxml2-{$ref/@file}.html#{$ref/@name}"><xsl:value-of select="$token"/></a>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$token"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- dumps a string, making cross-reference links -->
  <xsl:template name="dumptext">
    <xsl:param name="text"/>
    <xsl:variable name="ctxt" select='.'/>
    <!-- <xsl:value-of select="$text"/> -->
    <xsl:for-each select="str:tokenize($text, ' &#9;')">
      <xsl:apply-templates select="$ctxt" mode='dumptoken'>
        <xsl:with-param name="token" select="string(.)"/>
      </xsl:apply-templates>
      <xsl:if test="position() != last()">
        <xsl:text> </xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

<!--

             The following builds the Synopsis section

-->
  <xsl:template mode="synopsis" match="function">
    <xsl:variable name="name" select="string(@name)"/>
    <xsl:variable name="nlen" select="string-length($name)"/>
    <xsl:variable name="tlen" select="string-length(return/@type)"/>
    <xsl:variable name="blen" select="(($nlen + 8) - (($nlen + 8) mod 8)) + (($tlen + 8) - (($tlen + 8) mod 8))"/>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="return/@type"/>
    </xsl:call-template>
    <xsl:text>&#9;</xsl:text>
    <a href="#{@name}"><xsl:value-of select="@name"/></a>
    <xsl:if test="$blen - 40 &lt; -8">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:if test="$blen - 40 &lt; 0">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:text>&#9;(</xsl:text>
    <xsl:if test="not(arg)">
      <xsl:text>void</xsl:text>
    </xsl:if>
    <xsl:for-each select="arg">
      <xsl:call-template name="dumptext">
        <xsl:with-param name="text" select="@type"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:if test="position() != last()">
        <xsl:text>, </xsl:text><br/>
	<xsl:if test="$blen - 40 &gt; 8">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:if test="$blen - 40 &gt; 0">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:text>&#9;&#9;&#9;&#9;&#9; </xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>);</xsl:text>
    <xsl:text>
</xsl:text>
  </xsl:template>

  <xsl:template mode="synopsis" match="functype">
    <xsl:variable name="name" select="string(@name)"/>
    <xsl:variable name="nlen" select="string-length($name)"/>
    <xsl:variable name="tlen" select="string-length(return/@type)"/>
    <xsl:variable name="blen" select="(($nlen + 8) - (($nlen + 8) mod 8)) + (($tlen + 8) - (($tlen + 8) mod 8))"/>
    <xsl:text>typedef </xsl:text>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="return/@type"/>
    </xsl:call-template>
    <xsl:text> </xsl:text>
    <a href="#{@name}"><xsl:value-of select="@name"/></a>
    <xsl:if test="$blen - 40 &lt; -8">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:if test="$blen - 40 &lt; 0">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:text>&#9;(</xsl:text>
    <xsl:if test="not(arg)">
      <xsl:text>void</xsl:text>
    </xsl:if>
    <xsl:for-each select="arg">
      <xsl:call-template name="dumptext">
        <xsl:with-param name="text" select="@type"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:if test="position() != last()">
        <xsl:text>, </xsl:text><br/>
	<xsl:if test="$blen - 40 &gt; 8">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:if test="$blen - 40 &gt; 0">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:text>&#9;&#9;&#9;&#9;&#9; </xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>);</xsl:text>
    <xsl:text>
</xsl:text>
  </xsl:template>

  <xsl:template mode="synopsis" match="exports[@type='function']">
    <xsl:variable name="def" select="key('symbols',@symbol)"/>
    <xsl:apply-templates mode="synopsis" select="$def"/>
  </xsl:template>

  <xsl:template mode="synopsis" match="exports[@type='typedef']">
    <xsl:text>typedef </xsl:text>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="string(key('symbols',@symbol)/@type)"/>
    </xsl:call-template>
    <xsl:text> </xsl:text>
    <a href="#{@symbol}"><xsl:value-of select="@symbol"/></a>
    <xsl:text>;
</xsl:text>
  </xsl:template>

  <xsl:template mode="synopsis" match="exports[@type='macro']">
    <xsl:variable name="def" select="key('symbols',@symbol)"/>
    <xsl:text>#define </xsl:text>
    <a href="#{@symbol}"><xsl:value-of select="@symbol"/></a>
    <xsl:if test="$def/arg">
      <xsl:text>(</xsl:text>
      <xsl:for-each select="$def/arg">
        <xsl:value-of select="@name"/>
	<xsl:if test="position() != last()">
	  <xsl:text>, </xsl:text>
	</xsl:if>
      </xsl:for-each>
      <xsl:text>)</xsl:text>
    </xsl:if>
    <xsl:text>;
</xsl:text>
  </xsl:template>
  <xsl:template mode="synopsis" match="exports[@type='enum']">
  </xsl:template>
  <xsl:template mode="synopsis" match="exports[@type='struct']">
  </xsl:template>

<!--

             The following builds the Details section

-->
  <xsl:template mode="details" match="struct">
    <xsl:variable name="name" select="string(@name)"/>
    <div class="refsect2" lang="en">
    <h3><a name="{$name}">Structure </a><xsl:value-of select="$name"/></h3>
    <pre class="programlisting">
    <xsl:value-of select="@type"/><xsl:text> {
</xsl:text>
    <xsl:if test="not(field)">
      <xsl:text>The content of this structure is not made public by the API.
</xsl:text>
    </xsl:if>
    <xsl:for-each select="field">
        <xsl:text>    </xsl:text>
	<xsl:call-template name="dumptext">
	  <xsl:with-param name="text" select="@type"/>
	</xsl:call-template>
	<xsl:text>&#9;</xsl:text>
	<xsl:value-of select="@name"/>
	<xsl:if test="@info != ''">
	  <xsl:text>&#9;: </xsl:text>
	  <xsl:call-template name="dumptext">
	    <xsl:with-param name="text" select="substring(@info, 1, 70)"/>
	  </xsl:call-template>
	</xsl:if>
	<xsl:text>
</xsl:text>
    </xsl:for-each>
    <xsl:text>} </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>;
</xsl:text>
    </pre>
    <p>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="info"/>
    </xsl:call-template>
    </p><xsl:text>
</xsl:text>
    </div><hr/>
  </xsl:template>

  <xsl:template mode="details" match="typedef[@type != 'enum']">
    <xsl:variable name="name" select="string(@name)"/>
    <div class="refsect2" lang="en">
    <h3><a name="{$name}">Typedef </a><xsl:value-of select="$name"/></h3>
    <pre class="programlisting">
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="string(@type)"/>
    </xsl:call-template>
    <xsl:text> </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>;
</xsl:text>
    </pre>
    <p>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="info"/>
    </xsl:call-template>
    </p><xsl:text>
</xsl:text>
    </div><hr/>
  </xsl:template>

  <xsl:template mode="details" match="variable">
    <xsl:variable name="name" select="string(@name)"/>
    <div class="refsect2" lang="en">
    <h3><a name="{$name}">Variable </a><xsl:value-of select="$name"/></h3>
    <pre class="programlisting">
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="string(@type)"/>
    </xsl:call-template>
    <xsl:text> </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>;
</xsl:text>
    </pre>
    <p>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="info"/>
    </xsl:call-template>
    </p><xsl:text>
</xsl:text>
    </div><hr/>
  </xsl:template>

  <xsl:template mode="details" match="typedef[@type = 'enum']">
    <xsl:variable name="name" select="string(@name)"/>
    <div class="refsect2" lang="en">
    <h3><a name="{$name}">Enum </a><xsl:value-of select="$name"/></h3>
    <pre class="programlisting">
    <xsl:text>enum </xsl:text>
    <a href="#{$name}"><xsl:value-of select="$name"/></a>
    <xsl:text> {
</xsl:text>
    <xsl:for-each select="/api/symbols/enum[@type=$name]">
      <xsl:sort select="@value" data-type="number" order="ascending"/>
      <xsl:text>    </xsl:text>
      <a name="{@name}"><xsl:value-of select="@name"/></a>
      <xsl:if test="@value">
        <xsl:text> = </xsl:text>
	<xsl:value-of select="@value"/>
      </xsl:if>
      <xsl:if test="@info">
        <xsl:text> /* </xsl:text>
	<xsl:value-of select="@info"/>
        <xsl:text> */</xsl:text>
      </xsl:if>
      <xsl:text>
</xsl:text>
    </xsl:for-each>
    <xsl:text>};
</xsl:text>
    </pre>
    <p>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="info"/>
    </xsl:call-template>
    </p><xsl:text>
</xsl:text>
    </div><hr/>
  </xsl:template>

  <xsl:template mode="details" match="macro">
    <xsl:variable name="name" select="string(@name)"/>
    <div class="refsect2" lang="en">
    <h3><a name="{$name}">Macro </a><xsl:value-of select="$name"/></h3>
    <pre class="programlisting">
    <xsl:text>#define </xsl:text>
    <a href="#{$name}"><xsl:value-of select="$name"/></a>
    <xsl:if test="arg">
      <xsl:text>(</xsl:text>
      <xsl:for-each select="arg">
        <xsl:value-of select="@name"/>
	<xsl:if test="position() != last()">
	  <xsl:text>, </xsl:text>
	</xsl:if>
      </xsl:for-each>
      <xsl:text>)</xsl:text>
    </xsl:if>
    <xsl:text>;
</xsl:text>
    </pre>
    <p>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="info"/>
    </xsl:call-template>
    </p>
    <xsl:if test="arg">
      <div class="variablelist"><table border="0"><col align="left"/><tbody>
      <xsl:for-each select="arg">
        <tr>
          <td><span class="term"><i><tt><xsl:value-of select="@name"/></tt></i>:</span></td>
	  <td>
	    <xsl:call-template name="dumptext">
	      <xsl:with-param name="text" select="@info"/>
	    </xsl:call-template>
	  </td>
        </tr>
      </xsl:for-each>
      </tbody></table></div>
    </xsl:if>
    <xsl:text>
</xsl:text>
    </div><hr/>
  </xsl:template>

  <xsl:template mode="details" match="function">
    <xsl:variable name="name" select="string(@name)"/>
    <xsl:variable name="nlen" select="string-length($name)"/>
    <xsl:variable name="tlen" select="string-length(return/@type)"/>
    <xsl:variable name="blen" select="(($nlen + 8) - (($nlen + 8) mod 8)) + (($tlen + 8) - (($tlen + 8) mod 8))"/>
    <div class="refsect2" lang="en">
    <h3><a name="{$name}"></a><xsl:value-of select="$name"/> ()</h3>
    <pre class="programlisting">
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="return/@type"/>
    </xsl:call-template>
    <xsl:text>&#9;</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:if test="$blen - 40 &lt; -8">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:if test="$blen - 40 &lt; 0">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:text>&#9;(</xsl:text>
    <xsl:if test="not(arg)">
      <xsl:text>void</xsl:text>
    </xsl:if>
    <xsl:for-each select="arg">
      <xsl:call-template name="dumptext">
        <xsl:with-param name="text" select="@type"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:if test="position() != last()">
        <xsl:text>, </xsl:text><br/>
	<xsl:if test="$blen - 40 &gt; 8">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:if test="$blen - 40 &gt; 0">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:text>&#9;&#9;&#9;&#9;&#9; </xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>)</xsl:text><br/>
    <xsl:text>
</xsl:text>
    </pre>
    <p>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="info"/>
    </xsl:call-template>
    </p><xsl:text>
</xsl:text>
    <xsl:if test="arg | return/@info">
      <div class="variablelist"><table border="0"><col align="left"/><tbody>
      <xsl:for-each select="arg">
        <tr>
          <td><span class="term"><i><tt><xsl:value-of select="@name"/></tt></i>:</span></td>
	  <td>
	    <xsl:call-template name="dumptext">
	      <xsl:with-param name="text" select="@info"/>
	    </xsl:call-template>
	  </td>
        </tr>
      </xsl:for-each>
      <xsl:if test="return/@info">
        <tr>
          <td><span class="term"><i><tt>Returns</tt></i>:</span></td>
	  <td>
	    <xsl:call-template name="dumptext">
	      <xsl:with-param name="text" select="return/@info"/>
	    </xsl:call-template>
	  </td>
        </tr>
      </xsl:if>
      </tbody></table></div>
    </xsl:if>
    </div><hr/>
  </xsl:template>

  <xsl:template mode="details" match="functype">
    <xsl:variable name="name" select="string(@name)"/>
    <xsl:variable name="nlen" select="string-length($name)"/>
    <xsl:variable name="tlen" select="string-length(return/@type)"/>
    <xsl:variable name="blen" select="(($nlen + 8) - (($nlen + 8) mod 8)) + (($tlen + 8) - (($tlen + 8) mod 8))"/>
    <div class="refsect2" lang="en">
    <h3><a name="{$name}"></a>Function type <xsl:value-of select="$name"/> </h3>
    <pre class="programlisting">
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="return/@type"/>
    </xsl:call-template>
    <xsl:text>&#9;</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:if test="$blen - 40 &lt; -8">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:if test="$blen - 40 &lt; 0">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:text>&#9;(</xsl:text>
    <xsl:if test="not(arg)">
      <xsl:text>void</xsl:text>
    </xsl:if>
    <xsl:for-each select="arg">
      <xsl:call-template name="dumptext">
        <xsl:with-param name="text" select="@type"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:if test="position() != last()">
        <xsl:text>, </xsl:text><br/>
	<xsl:if test="$blen - 40 &gt; 8">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:if test="$blen - 40 &gt; 0">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:text>&#9;&#9;&#9;&#9;&#9; </xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>)</xsl:text><br/>
    <xsl:text>
</xsl:text>
    </pre>
    <p>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="info"/>
    </xsl:call-template>
    </p><xsl:text>
</xsl:text>
    <xsl:if test="arg | return/@info">
      <div class="variablelist"><table border="0"><col align="left"/><tbody>
      <xsl:for-each select="arg">
        <tr>
          <td><span class="term"><i><tt><xsl:value-of select="@name"/></tt></i>:</span></td>
	  <td>
	    <xsl:call-template name="dumptext">
	      <xsl:with-param name="text" select="@info"/>
	    </xsl:call-template>
	  </td>
        </tr>
      </xsl:for-each>
      <xsl:if test="return/@info">
        <tr>
          <td><span class="term"><i><tt>Returns</tt></i>:</span></td>
	  <td>
	    <xsl:call-template name="dumptext">
	      <xsl:with-param name="text" select="return/@info"/>
	    </xsl:call-template>
	  </td>
        </tr>
      </xsl:if>
      </tbody></table></div>
    </xsl:if>
    </div><hr/>
  </xsl:template>

<!--

             The following builds the general.html page

-->
  <xsl:template name="generate_general">
    <xsl:variable name="next" select="string(/api/files/file[position()=1]/@name)"/>
    <xsl:document xmlns="" href="general.html" method="xml" indent="yes" encoding="UTF-8">
      <html>
        <head>
	  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>
	  <title><xsl:value-of select="concat(@name, ': ', summary)"/></title>
	  <meta name="generator" content="Libxml2 devhelp stylesheet"/>
	  <link rel="start" href="index.html" title="libxml2 Reference Manual"/>
	  <link rel="up" href="index.html" title="libxml2 Reference Manual"/>
	  <link rel="stylesheet" href="style.css" type="text/css"/>
	  <link rel="chapter" href="index.html" title="libxml2 Reference Manual"/>
        </head>
	<body bgcolor="white" text="black" link="#0000FF" vlink="#840084" alink="#0000FF">

          <table class="navigation" width="100%" summary="Navigation header" cellpadding="2" cellspacing="2">
	    <tr valign="middle">
              <td><a accesskey="u" href="index.html"><img src="up.png" width="24" height="24" border="0" alt="Up"/></a></td>
              <td><a accesskey="h" href="index.html"><img src="home.png" width="24" height="24" border="0" alt="Home"/></a></td>
	      <xsl:if test="$next != ''">
		<td><a accesskey="n" href="libxml2-{$next}.html"><img src="right.png" width="24" height="24" border="0" alt="Next"/></a></td>
	      </xsl:if>
              <th width="100%" align="center">libxml2 Reference Manual</th>
            </tr>
	  </table>
	  <h2><span class="refentrytitle">libxml2 API Modules</span></h2>
	  <p>
	  <xsl:for-each select="/api/files/file">
	    <a href="libxml2-{@name}.html"><xsl:value-of select="@name"/></a> - <xsl:value-of select="summary"/><br/>
	  </xsl:for-each>
	  </p>
	</body>
      </html>
    </xsl:document>
  </xsl:template>

<!--

             The following builds the index.html page

-->
  <xsl:template name="generate_index">
    <xsl:document xmlns="" href="index.html" method="xml" indent="yes" encoding="UTF-8">
      <html>
        <head>
	  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>
	  <title>libxml2 Reference Manual</title>
	  <meta name="generator" content="Libxml2 devhelp stylesheet"/>
	  <link rel="stylesheet" href="style.css" type="text/css"/>
        </head>
	<body bgcolor="white" text="black" link="#0000FF" vlink="#840084" alink="#0000FF">

          <table class="navigation" width="100%" summary="Navigation header" cellpadding="2" cellspacing="2">
	    <tr valign="middle">
              <td><a accesskey="h" href="index.html"><img src="home.png" width="24" height="24" border="0" alt="Home"/></a></td>
	      <td><a accesskey="n" href="general.html"><img src="right.png" width="24" height="24" border="0" alt="Next"/></a></td>
              <th width="100%" align="center">libxml2 Reference Manual</th>
            </tr>
	  </table>
	  <h2><span class="refentrytitle">libxml2 Reference Manual</span></h2>
<p>Libxml2 is the XML C parser and toolkit developed for the Gnome project
(but usable outside of the Gnome platform), it is free software available
under the <a href="http://www.opensource.org/licenses/mit-license.html">MIT
License</a>. XML itself is a metalanguage to design markup languages, i.e.
text language where semantic and structure are added to the content using
extra "markup" information enclosed between angle brackets. HTML is the most
well-known markup language. Though the library is written in C <a href="http://xmlsoft.org/python.html">a variety of language bindings</a> make it available in
other environments.</p>
<p>Libxml2 implements a number of existing standards related to markup
languages:</p>
<ul><li>the XML standard: <a href="http://www.w3.org/TR/REC-xml">http://www.w3.org/TR/REC-xml</a></li>
<li>Namespaces in XML: <a href="http://www.w3.org/TR/REC-xml-names/">http://www.w3.org/TR/REC-xml-names/</a></li>
<li>XML Base: <a href="http://www.w3.org/TR/xmlbase/">http://www.w3.org/TR/xmlbase/</a></li>
<li><a href="http://www.cis.ohio-state.edu/rfc/rfc2396.txt">RFC 2396</a> :
Uniform Resource Identifiers <a href="http://www.ietf.org/rfc/rfc2396.txt">http://www.ietf.org/rfc/rfc2396.txt</a></li>
<li>XML Path Language (XPath) 1.0: <a href="http://www.w3.org/TR/xpath">http://www.w3.org/TR/xpath</a></li>
<li>HTML4 parser: <a href="http://www.w3.org/TR/html401/">http://www.w3.org/TR/html401/</a></li>
<li>XML Pointer Language (XPointer) Version 1.0: <a href="http://www.w3.org/TR/xptr">http://www.w3.org/TR/xptr</a></li>
<li>XML Inclusions (XInclude) Version 1.0: <a href="http://www.w3.org/TR/xinclude/">http://www.w3.org/TR/xinclude/</a></li>
<li>ISO-8859-x encodings, as well as <a href="http://www.cis.ohio-state.edu/rfc/rfc2044.txt">rfc2044</a> [UTF-8]
and <a href="http://www.cis.ohio-state.edu/rfc/rfc2781.txt">rfc2781</a>
[UTF-16] Unicode encodings, and more if using iconv support</li>
<li>part of SGML Open Technical Resolution TR9401:1997</li>
<li>XML Catalogs Working Draft 06 August 2001: <a href="http://www.oasis-open.org/committees/entity/spec-2001-08-06.html">http://www.oasis-open.org/committees/entity/spec-2001-08-06.html</a></li>
<li>Canonical XML Version 1.0: <a href="http://www.w3.org/TR/xml-c14n">http://www.w3.org/TR/xml-c14n</a>
and the Exclusive XML Canonicalization CR draft <a href="http://www.w3.org/TR/xml-exc-c14n">http://www.w3.org/TR/xml-exc-c14n</a></li>
<li>Relax NG, ISO/IEC 19757-2:2003, <a href="http://www.oasis-open.org/committees/relax-ng/spec-20011203.html">http://www.oasis-open.org/committees/relax-ng/spec-20011203.html</a></li>
<li>W3C XML Schemas Part 2: Datatypes <a href="http://www.w3.org/TR/2001/REC-xmlschema-2-20010502/">REC 02 May
2001</a></li>
<li>W3C <a href="http://www.w3.org/TR/xml-id/">xml:id</a> Working Draft 7
April 2004</li>
</ul>
	  <p> As a result the <a href="general.html">libxml2 API</a> is very
	      large. If you get lost searching for some specific API use
	      <a href="http://xmlsoft.org/search.php">the online search
	      engine</a> hosted on <a href="http://xmlsoft.org/">xmlsoft.org</a>
	      the libxml2 and libxslt project page. </p>
	</body>
      </html>
    </xsl:document>
  </xsl:template>

</xsl:stylesheet>
