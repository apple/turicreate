## Turi Create Visualization Gallery

Click on a thumbnail to see the code and a larger image.

<table>
  <tr>
    <td>
      <a href="#sframeshow"><img src="images/sframe_show.png"></a>
    </td>
    <td>
      <a href="#sframeexplore"><img src="images/sframe_explore_with_images.png"></a>
    </td>
    <td>
      <a href="#sarrayshow-intfloat"><img src="images/numeric_histogram.png"></a>
    </td>
    <td>
      <a href="#sarrayshow-str"><img src="images/categorical_histogram.png"></a>
    </td>
  </tr>
  <tr>
    <td>
      <a href="#scatter-plot"><img src="images/scatter_plot.png"></a>
    </td>
    <td>
      <a href="#numeric-heat-map"><img src="images/numeric_heat_map.png"></a>
    </td>
    <td>
      <a href="#categorical-heat-map"><img src="images/categorical_heat_map.png"></a>
    </td>
    <td>
      <a href="#box-plot"><img src="images/box_plot_2.png"></a>
    </td>
  </tr>
</table>

#### Examples

##### SFrame.show

```python
# Summarizes and shows the summary of each column in sf
sf.show()
```
![sf.show()](images/sframe_show.png)

##### SFrame.explore

```python
# Opens an interactive exploration of the data in sf
sf.explore()
```
![sf.explore()](images/sframe_explore_with_images.png)

##### SArray.show (int/float)

```python
# Summarizes and shows the summary of a numeric SArray
sa.show(title='Normalized Lines Changed')
```
![sa.show()](images/numeric_histogram.png)

##### SArray.show (str)

```python
# Summarizes and shows the summary of a categorical SArray
sa.show()
```
![sa.show()](images/categorical_histogram.png)

##### Scatter plot

```python
# Assumes `sa1` and `sa2` are both numeric (int/float) SArrays <= 5,000 rows
turicreate.show(sa1, sa2, xlabel='Actual Change', ylabel='Predicted Change')
```
![scatter plot](images/scatter_plot.png)

##### Numeric heat map

```python
# Assumes `sa1` and `sa2` are both numeric (int/float) SArrays > 5,000 rows
turicreate.show(sa1, sa2, xlabel='Lines Added', ylabel='Lines Removed')
```
![numeric heat map](images/numeric_heat_map.png)

##### Categorical heat map

```python
# Assumes `sa1` and `sa2` are both categorical (str) SArrays
turicreate.show(sa1, sa2, xlabel='Label', ylabel='Cap Shape')
```
![numeric heat map](images/categorical_heat_map.png)

##### Box plot

```python
# Assumes `sa1` is numeric (int/float) and `sa2` is categorical (str),
# or alternatively, `sa2` is numeric and `sa1` is categorical.
turicreate.show(sa1, sa2, xlabel='Based on Style', ylabel='Normalized Lines Changed')
```
![box plot](images/box_plot_2.png)
