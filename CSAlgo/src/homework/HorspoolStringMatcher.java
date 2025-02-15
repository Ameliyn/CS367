package homework;

//@Author Skye Russ
public class HorspoolStringMatcher {

    private char[] needle;
    private int[] shiftTable;

    public HorspoolStringMatcher(String needle){
        this.needle = needle.toCharArray();
        GenerateShiftTable();
    }

    private void GenerateShiftTable(){
        shiftTable = new int[256];
        for(int i = 0; i < 256; i++){
            shiftTable[i] = getShift((char)i);
        }
    }

    /**
     * returns how many you can jump before the next occurrence of this letter
     * @param c
     * @return
     */
    public int getShift(char c){
        int firstIndex = needle.length;
        for(int i = needle.length-2; i >= 0; i--){
            if(needle[i] == c){
                firstIndex = needle.length - 1 - i;
                break;
            }
        }
        return firstIndex;
    }

    /**
     * match returns the first index of the needle in the haystack
     * @param haystack string to be searched
     * @return first index of needle in haystack
     */
    public int match(String haystack){
        int skip = 0;
        while(haystack.length() - skip >= needle.length){
            if(same(haystack.substring(skip).toCharArray(), needle, needle.length)){
                return skip;
            }
            skip += shiftTable[haystack.toCharArray()[skip + needle.length - 1]];
        }
        return -1;
    }

    private boolean same(char[] a, char[] b, int length){
        int i = length - 1;
        while(a[i] == b[i]){
            if(i == 0) return true;
            i--;
        }
        return false;
    }
}
